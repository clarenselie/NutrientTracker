# Nutrient Tracker

Nutrient Tracker is a C++ desktop application built with Qt Widgets. It lets a user type foods such as `2 eggs, 1 cup rice, banana`, matches those foods to a USDA-backed SQLite database, converts the amounts into grams, and displays calories, macros, vitamins, minerals, and daily totals.

## Features

- Parse natural food input into food names, quantities, and units
- Search foods in a local SQLite nutrition database
- Convert grams, kilograms, ounces, pounds, cups, servings, and units
- Calculate calories, protein, carbs, fat, fiber, vitamins, and minerals
- Show a daily food log and nutrient breakdown dashboard
- Handle unmatched foods, invalid quantities, and missing database setup gracefully

## Requirements

- CMake 3.16 or newer
- C++17 compiler
- Qt 6 or Qt 5 with these modules:
  - Qt Widgets
  - Qt SQL
- SQLite driver for Qt, usually included with desktop Qt kits
- Qt Creator is recommended

## Project Structure

```text
src/main.cpp          Starts the app or database builder
src/models/          Shared food, nutrient, and analysis data structs
src/data/            SQLite database setup and USDA CSV importer
src/core/            Input parsing, food matching, unit conversion, nutrient math
src/ui/              Qt windows, pages, dashboard, and widgets
docs/                UML and algorithm diagrams
scripts/             Database helper scripts
data/README.md       Notes about nutrition data setup
```

## Important Database Note

The full nutrition database is not included in this GitHub repository because it is too large.

Ignored files include:

```text
data/nutrient_tracker.sqlite
data/usda/
```

To run the app with real food results, you need one of these:

1. A prebuilt `nutrient_tracker.sqlite` file placed at:

   ```text
   data/nutrient_tracker.sqlite
   ```

2. The raw USDA FoodData Central CSV files placed in:

   ```text
   data/usda/
   ```

Then build the database using the instructions below.

USDA data source: https://fdc.nal.usda.gov/download-datasets.html

## Open in Qt Creator

1. Clone or download this repository.
2. Open Qt Creator.
3. Choose `File > Open File or Project`.
4. Select this file from the project folder:

   ```text
   CMakeLists.txt
   ```

5. Select a desktop Qt kit.
6. Use a fresh build directory, for example:

   ```text
   build-qtcreator
   ```

7. Click `Configure Project`.
8. Build and run the `NutrientTracker` target.

If Qt Creator cannot find Qt, set `CMAKE_PREFIX_PATH` to your Qt installation. On some macOS Homebrew setups, this may look like:

```bash
-DCMAKE_PREFIX_PATH=/opt/homebrew/lib/cmake/Qt6
```

## Build from Terminal

From the project folder:

```bash
cmake -S . -B build-qtcreator
cmake --build build-qtcreator
```

On macOS, the app will be built at:

```text
build-qtcreator/NutrientTracker.app
```

## Build the Food Database

If you have the raw USDA CSV export in `data/usda/`, build the SQLite database with:

```bash
./build-qtcreator/NutrientTracker.app/Contents/MacOS/NutrientTracker --build-food-db data/usda data/nutrient_tracker.sqlite
```

After `data/nutrient_tracker.sqlite` is created, rebuild the app:

```bash
cmake --build build-qtcreator
```

CMake will copy the SQLite database into the app bundle. When the app starts, it installs the bundled database into the user's app data folder.

## Example Inputs

```text
2 eggs, 1 cup rice, banana
1 serving sun chips
1.5 cups oatmeal, 2 bananas
```

## Troubleshooting

If you copied the project folder and Qt Creator gives a CMake cache warning, delete the old build folder or choose a new build directory.

If the app opens but food search does not work, make sure this file exists:

```text
data/nutrient_tracker.sqlite
```

If you see an `appman-controller` error in Qt Creator, switch the run/deploy configuration back to the normal desktop run configuration instead of Qt Application Manager.

## Credits

- GUI framework: Qt
- Local database: SQLite
- Nutrition data: USDA FoodData Central
