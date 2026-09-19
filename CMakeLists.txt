cmake_minimum_required(VERSION 3.16)
project(VehicleConnectivitySDK LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug)
endif()

add_compile_options(-Wall -Wextra)

add_library(vcsdk
    src/net/TcpSocketClient.cpp
    src/net/SerialPortClient.cpp
    src/security/CommandSecurityPolicy.cpp
    src/configuration/Config.cpp
    src/connectors/virtual/VirtualConnector.cpp
    src/connectors/tcp/TcpConnector.cpp
    src/connectors/serial/SerialConnector.cpp
    src/connectors/socketcan/SocketCanConnector.cpp
    src/connectors/bluetooth/BluetoothConnector.cpp
)
target_include_directories(vcsdk PUBLIC include)
find_package(Threads REQUIRED)
find_package(yaml-cpp REQUIRED)
target_link_libraries(vcsdk PUBLIC Threads::Threads bluetooth yaml-cpp)

add_executable(demo_virtual_connector examples/demo_virtual_connector.cpp)
target_link_libraries(demo_virtual_connector PRIVATE vcsdk)

enable_testing()
find_package(GTest REQUIRED)

function(add_contract_test name source)
    add_executable(${name} ${source})
    target_link_libraries(${name} PRIVATE vcsdk GTest::gtest GTest::gtest_main)
    add_test(NAME ${name} COMMAND ${name})
endfunction()

add_contract_test(test_virtual_connector       tests/contract/test_virtual_connector.cpp)
add_contract_test(test_tcp_connector           tests/contract/test_tcp_connector.cpp)
add_contract_test(test_serial_connector        tests/contract/test_serial_connector.cpp)
add_contract_test(test_socketcan_connector     tests/contract/test_socketcan_connector.cpp)
add_contract_test(test_bluetooth_connector     tests/contract/test_bluetooth_connector.cpp)
add_contract_test(test_retry_policy            tests/unit/test_retry_policy.cpp)
add_contract_test(test_reconnect_helper        tests/unit/test_reconnect_helper.cpp)
add_contract_test(test_command_security_policy tests/unit/test_command_security_policy.cpp)
add_contract_test(test_local_buffer            tests/unit/test_local_buffer.cpp)
add_contract_test(test_rate_limiter            tests/unit/test_rate_limiter.cpp)
add_contract_test(test_metrics                 tests/unit/test_metrics.cpp)
add_contract_test(test_structured_logger       tests/unit/test_structured_logger.cpp)
add_contract_test(test_config_and_factory      tests/unit/test_config_and_factory.cpp)
add_contract_test(test_performance             tests/performance/test_performance.cpp)
add_contract_test(test_integration             tests/integration/test_integration.cpp)