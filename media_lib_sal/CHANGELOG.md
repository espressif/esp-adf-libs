# Changelog

## v1.0.0

### Features

- Registerd the default OS adapter with the `constructor` attribute so APIs such as `media_lib_malloc()` work even if `media_lib_add_default_adapter()` is not explicitly called

## v0.9.3

### Bugfix

- Added WHOLE_ARCHIVE attribute to avoid some function is not linked

## v0.9.2

### Features

- Enhanced test app to cover more cases
- Added support for IDFv6.x

## v0.9.1

### Bugfix

- Fixed wrongly return success when TLS connect timeout

## v0.9.0

- Initial version of `media_lib_sal`
