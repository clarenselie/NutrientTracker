# Nutrient Tracker

A Qt Widgets nutrient tracker built in C++.

## What it does

- Parses meal text such as `2 eggs, 1 cup rice, banana`
- Matches foods from a bundled USDA-backed SQLite database
- Converts quantities to grams
- Calculates macro, mineral, vitamin, and advanced nutrient totals
- Shows unmatched foods so they can be fixed quickly

## Open in Qt Creator

1. Open Qt Creator.
2. Choose `Open Project`.
3. Select [CMakeLists.txt](/Users/clarensguibbs/Documents/New project 3/CMakeLists.txt).
4. Configure with a desktop Qt kit that includes `Qt Widgets` and `Qt SQL`.
5. Build and run `NutrientTracker`.

## Food database setup

The desktop app should not import the raw 3 GB USDA CSV export on user startup.
Build the SQLite food database once as the developer, then bundle that finished file.

From the project folder:

```bash
./build/NutrientTracker.app/Contents/MacOS/NutrientTracker --build-food-db data/usda data/nutrient_tracker.sqlite
```

After that file exists, rebuild in Qt Creator. CMake copies
`data/nutrient_tracker.sqlite` into the app bundle. On user startup, the app
silently installs that SQLite database into the user's app data folder.

## Example input

- `2 eggs, 1 cup rice, banana`
- `1 serving sun chips`
- `1.5 cups oatmeal, 2 bananas`

## Notes

Keep raw USDA CSV files in `data/usda` only for building the SQLite database.
Do not ship the raw CSV folder as the runtime database.
