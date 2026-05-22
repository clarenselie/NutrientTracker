cmake_minimum_required(VERSION 3.16)

project(NutrientTracker VERSION 0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Widgets Sql)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets Sql)

set(PROJECT_SOURCES
    src/main.cpp
    src/models/NutrientTypes.h
    src/models/DomainTypes.h
    src/data/DatabaseManager.h
    src/data/DatabaseManager.cpp
    src/data/USDAImporter.h
    src/data/USDAImporter.cpp
    src/core/CoreComponent.h
    src/core/InputParser.h
    src/core/InputParser.cpp
    src/core/FoodMatcher.h
    src/core/FoodMatcher.cpp
    src/core/UnitConverter.h
    src/core/UnitConverter.cpp
    src/core/NutrientEngine.h
    src/core/NutrientEngine.cpp
    src/core/MealAnalyzer.h
    src/core/MealAnalyzer.cpp
    src/ui/WelcomePage.h
    src/ui/WelcomePage.cpp
    src/ui/MainPage.h
    src/ui/MainPage.cpp
    src/ui/AboutPage.h
    src/ui/AboutPage.cpp
    src/ui/MainWindow.h
    src/ui/MainWindow.cpp
)

if(${QT_VERSION_MAJOR} GREATER_EQUAL 6)
    qt_add_executable(NutrientTracker
        MANUAL_FINALIZATION
        ${PROJECT_SOURCES}
    )
else()
    add_executable(NutrientTracker
        ${PROJECT_SOURCES}
    )
endif()

target_include_directories(NutrientTracker PRIVATE
    src
    src/core
    src/data
    src/models
    src/ui
)

target_link_libraries(NutrientTracker PRIVATE
    Qt${QT_VERSION_MAJOR}::Widgets
    Qt${QT_VERSION_MAJOR}::Sql
)

set(PREBUILT_FOOD_DATABASE "${CMAKE_CURRENT_SOURCE_DIR}/data/nutrient_tracker.sqlite")
if(APPLE)
    add_custom_command(TARGET NutrientTracker POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E remove_directory "$<TARGET_BUNDLE_CONTENT_DIR:NutrientTracker>/Resources/usda"
    )
elseif(NOT APPLE)
    add_custom_command(TARGET NutrientTracker POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E remove_directory "$<TARGET_FILE_DIR:NutrientTracker>/data/usda"
    )
endif()

if(EXISTS "${PREBUILT_FOOD_DATABASE}")
    if(APPLE)
        add_custom_command(TARGET NutrientTracker POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different "${PREBUILT_FOOD_DATABASE}" "$<TARGET_BUNDLE_CONTENT_DIR:NutrientTracker>/Resources/nutrient_tracker.sqlite"
        )
    else()
        add_custom_command(TARGET NutrientTracker POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:NutrientTracker>/data"
            COMMAND ${CMAKE_COMMAND} -E copy_if_different "${PREBUILT_FOOD_DATABASE}" "$<TARGET_FILE_DIR:NutrientTracker>/data/nutrient_tracker.sqlite"
        )
    endif()
endif()

set_target_properties(NutrientTracker PROPERTIES
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)

if(${QT_VERSION_MAJOR} EQUAL 6)
    qt_finalize_executable(NutrientTracker)
endif()
