# Changelog

## v1.0.2

### Bugfix

- Uniformed remove the use of member `use_secure_element` above ESP-IDF v6.0

## v1.0.1

### Bugfix

- Removed `use_secure_element` from TLS config mapping for compatibility with ESP-IDF master after the field was removed from `esp_tls_cfg_t`

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
