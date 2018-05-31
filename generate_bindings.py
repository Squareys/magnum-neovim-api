#!/usr/bin/env python
"""
C++ source code generator to create bindings from Neovim API functions
using Magnum types

Based on the bindings/generate_bindings.py script from https://github.com/equalsraf/neovim-qt.
"""

import argparse
import msgpack
import sys, subprocess, os
import re
import jinja2
import datetime


def decutf8(inp):
    """
    Recursively decode bytes as utf8 into unicode
    """
    if isinstance(inp, bytes):
        return inp.decode('utf8')
    elif isinstance(inp, list):
        return [decutf8(x) for x in inp]
    elif isinstance(inp, dict):
        return {decutf8(key): decutf8(val) for key, val in inp.items()}
    else:
        return inp


def get_api_info(nvim):
    """
    Call the neovim binary to get the api info
    """
    args = [nvim, '--api-info']
    info = subprocess.check_output(args)
    return decutf8(msgpack.unpackb(info))


def generate_file(name, template_dir, outfile, **kw):
    from jinja2 import Environment, FileSystemLoader
    env = Environment(loader=FileSystemLoader(template_dir), trim_blocks=True)
    template = env.get_template(name)
    with open(outfile, 'w') as fp:
        fp.write(template.render(kw))


class UnsupportedType(Exception):
    pass


class NeovimTypeVal:
    """
    Representation for Neovim Parameter/Return
    """

    # msgpack simple types types
    SIMPLETYPES = {
            'Integer': 'Long',
            'Boolean': 'bool',
            'Float': 'Double',
            'Object': 'Object',
            'Array': 'Corrade::Containers::Array<Object>',
            'Dictionary': 'const std::unordered_map<std::string, Object>&',
        }

    # msgpack extension types
    EXTTYPES = {
            'Window': 'Long',
            'Buffer': 'Long',
            'Tabpage': 'Long',
        }

    PAIRTYPE = 'ArrayOf(Integer, 2)'

    # Unbound Array types
    UNBOUND_ARRAY = re.compile('ArrayOf\(\s*(\w+)\s*\)')
    ARRAY_OF = re.compile('ArrayOf\(\s*(\w+)\s*\)')

    def __init__(self, typename, name='', out=False):
        self.name = name
        self.neovim_type = typename
        self.ext = False
        self.native_type = NeovimTypeVal.nativeType(typename, out=out)
        self.elemtype = None

        if typename in self.SIMPLETYPES:
            pass
        elif typename in self.EXTTYPES:
            self.ext = True
        elif self.UNBOUND_ARRAY.match(typename):
            m = self.UNBOUND_ARRAY.match(typename)
            self.elemtype = m.groups()[0]
            self.native_elemtype = NeovimTypeVal.nativeType(self.elemtype, out=True)
        else:
            self.native_type = NeovimTypeVal.nativeType(typename, out)

        if typename == "Array":
            self.elemtype = "Object"
            self.native_elemtype = NeovimTypeVal.nativeType("Object", out=True)

    @classmethod
    def nativeType(cls, typename, out=False):
        """Return the native type for this Neovim type."""
        if typename == 'void':
            return typename
        if typename == 'String':
            return "std::string" if out else "const std::string&"
        elif typename in cls.SIMPLETYPES:
            return cls.SIMPLETYPES[typename]
        elif typename in cls.EXTTYPES:
            return cls.EXTTYPES[typename]
        elif cls.UNBOUND_ARRAY.match(typename):
            m = cls.UNBOUND_ARRAY.match(typename)
            if out:
                return 'Corrade::Containers::Array<%s>' % cls.nativeType(m.groups()[0], out=True)
            else:
                return 'Corrade::Containers::ArrayView<%s>' % cls.nativeType(m.groups()[0], out=True)
        elif typename == cls.PAIRTYPE:
            return 'Vector2i'
        raise UnsupportedType(typename)


class Function:
    """
    Representation of a Neovim API Function
    """

    # Attributes names that we support, see src/function.c for details
    __KNOWN_ATTRIBUTES = set([
        'name', 'parameters', 'return_type', 'can_fail', 'deprecated_since',
        'since', 'method', 'async', 'impl_name', 'noeval', 'receives_channel_id'])

    def __init__(self, nvim_fun):
        self.valid = False
        self.fun = nvim_fun
        self.parameters = []
        self.name =  self.fun['name']
        try:
            self.return_type = NeovimTypeVal(self.fun['return_type'], out=True)
            for param in self.fun['parameters']:
                self.parameters.append(NeovimTypeVal(*param))
        except UnsupportedType as ex:
            print('Found unsupported type(%s) when adding function %s(), skipping' % (ex, self.name))
            return

        u_attrs = self.unknown_attributes()
        if u_attrs:
            print('Found unknown attributes for function %s: %s' % (self.name, u_attrs))

        self.argcount = len(self.parameters)

        # Build the argument string - makes it easier for the templates
        self.argstring = ', '.join(['%s %s' % (p.native_type, p.name) for p in self.parameters])
        self.valid = True

    def is_method(self):
        return self.fun.get('method', False)

    def is_async(self):
        return self.fun.get('async', False)

    def deprecated(self):
        return self.fun.get('deprecated_since', None)

    def unknown_attributes(self):
        return set(self.fun.keys()) - Function.__KNOWN_ATTRIBUTES

    def real_signature(self):
        return '%s %s(%s)' % (self.return_type.native_type, self.name, self.argstring)

    def signature(self):
        params = ', '.join(['%s %s' % (p.neovim_type, p.name) for p in self.parameters])
        return '%s %s(%s)' % (self.return_type.neovim_type, self.name, params)


def main():
    parser = argparse.ArgumentParser(description="Generate C++ API bindings for the Neovim msgpack-rpc API")
    parser.add_argument("-n", "--nvim", help="path to nvim executable", default="nvim")
    parser.add_argument("-t", "--template-dir", help="template directory", default="template")
    parser.add_argument("-o", "--output", help="output directory", default=".")
    parser.add_argument("-d", "--with-deprecated", help="generate deprecated functions", action="store_true")
    args = parser.parse_args()

    nvim = args.nvim
    template_dir = args.template_dir
    outpath = args.output

    try:
        api = get_api_info(nvim)
    except subprocess.CalledProcessError as ex:
        print(ex)
        sys.exit(-1)

    api_level = api['version']['api_level'] if 'version' in api else 0

    print('Writing auto generated bindings (api{}) to {}'.format(api_level, outpath))

    if not os.path.exists(outpath):
        os.makedirs(outpath)

    files = os.listdir(template_dir)
    if not files:
        print("No .h or .cpp files in template directory", template_dir)

    # Filter out deprecated functions
    if not args.with_deprecated:
        api['functions'] = [f for f in api['functions'] if 'deprecated_since' not in f]

    functions = [Function(f) for f in api['functions'] if f['name'] != 'vim_get_api_info']
    exttypes = {typename: info['id'] for typename, info in api['types'].items()}
    env = {'date': datetime.datetime.now(),
           'functions': [f for f in functions if f.valid],
           'exttypes': exttypes,
           'api_level': api_level}

    for name in files:
        if name.startswith('.'):
            continue
        if not name.endswith('.h') and not name.endswith('.cpp'):
            continue

        fname, fext = os.path.splitext(name)
        fname = '{}{}{}'.format(fname, api_level, fext)
        outfile = os.path.join(outpath, fname)

        generate_file(name, template_dir, outfile, **env)


if __name__ == '__main__':
    main()

