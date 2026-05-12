Place the finished food database here before building the app:

- nutrient_tracker.sqlite

This file should already contain the USDA FoodData Central tables:

- foods
- branded_foods
- nutrients
- food_nutrients
- serving_sizes

At runtime, the app silently copies this SQLite file into the user's app data folder.
Do not ship the raw CSV folder as the runtime database.
