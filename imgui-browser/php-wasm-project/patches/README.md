# patches/

Place unified-diff `.patch` files here to automatically apply fixes to the PHP
source tree before compilation.

`scripts/apply-patches.sh` iterates every `*.patch` file in this directory in
alphabetical order and applies each one with `patch -p1`.

## Naming convention

```
001-fix-emscripten-compat.patch
002-disable-realpath-cache.patch
```

## Common patches needed for PHP WASM builds

| Patch | Purpose |
|-------|---------|
| `001-disable-realpath-cache.patch` | The realpath cache uses `stat()` calls that are slow or broken in the VFS |
| `002-no-fork.patch` | Remove `fork()`/`exec()` calls that don't exist in WASM |
| `003-fix-rand.patch` | Replace `/dev/urandom` reads with Emscripten's `getentropy` |

Refer to the [PHP WASM community resources](https://github.com/seanmorris/php-wasm)
for up-to-date patches targeting your PHP version.
