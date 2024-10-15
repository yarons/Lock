# Contributing

First of all, thank you for investing your time to improve this project!

## Issues

Found a bug? Just open an issue in [GitHub](https://github.com/konstantintutsch/Lock/issues)'s bug tracker.

Make sure to describe the idea you've had or the problem you've experienced in detail. Thank you!

## Pull Requests

You want to directly contribute to this project? Great! Open a pull request via [GitHub](https://github.com/konstantintutsch/Lock).

### Translations

This project is translated using GNU gettext. Translations for a specific language can be found at `po/<lang>.po`.

#### Adding a new language

If you want to add a language, append it to [LINGUAS](po/LINGUAS). Once finished, run the `translate` Just recipe.

```
just translate
```

The empty translations file is now accessible at `po/<lang>.po` and is ready to be translated.

### Code

The code of this project follows a few simple style rules. You can automatically apply them with the `format` Just recipe.

```
just format
```
