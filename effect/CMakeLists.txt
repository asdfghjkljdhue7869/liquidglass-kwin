cmake_minimum_required(VERSION 3.20)
project(liquidglasswash)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(KF6_MIN_VERSION "6.0.0")

find_package(ECM ${KF6_MIN_VERSION} REQUIRED NO_MODULE)
set(CMAKE_MODULE_PATH ${ECM_MODULE_PATH})

include(FeatureSummary)
include(KDEInstallDirs)
include(KDECMakeSettings)
include(KDECompilerSettings NO_POLICY_SCOPE)

find_package(Qt6 CONFIG REQUIRED COMPONENTS Core Gui DBus Widgets Quick)
find_package(KF6 ${KF6_MIN_VERSION} REQUIRED COMPONENTS Config CoreAddons WindowSystem)
find_package(KDecoration3 REQUIRED)
find_package(KWin REQUIRED COMPONENTS kwin)
find_package(XCB REQUIRED COMPONENTS XCB)

set(liquidglasswash_SOURCES liquidglass.cpp liquidglass.qrc main.cpp)
kconfig_add_kcfg_files(liquidglasswash_SOURCES liquidglassconfig.kcfgc)

kcoreaddons_add_plugin(liquidglasswash INSTALL_NAMESPACE "kwin/effects/plugins" SOURCES ${liquidglasswash_SOURCES})
target_link_libraries(liquidglasswash PRIVATE KWin::kwin KF6::ConfigGui KDecoration3::KDecoration XCB::XCB)

feature_summary(WHAT ALL FATAL_ON_MISSING_REQUIRED_PACKAGES)
