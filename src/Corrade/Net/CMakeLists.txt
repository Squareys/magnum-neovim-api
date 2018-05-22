find_package(Corrade REQUIRED Containers)

add_library(CorradeNet Socket.cpp)
target_link_libraries(CorradeNet Corrade::Containers Ws2_32 Mswsock AdvApi32)
