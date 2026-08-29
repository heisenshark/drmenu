# Repository Guidelines & Operational Rules

## Strict Safety Constraints (CRITICAL)
1. **Never load, unload, or reload Hyprland plugins**:
   - Do NOT run `hyprctl plugin load`
   - Do NOT run `hyprctl plugin unload`
   - Do NOT run `hyprctl plugin reload`
   - Do NOT run any commands or scripts that trigger plugin reloading.
   - All plugin reloading and unloading MUST be performed manually by the user.

2. **Never overwrite in-use `.so` plugin binaries in place**:
   - Overwriting a `.so` file while `dlopen()`-mapped in Hyprland causes `SIGBUS`/`SIGSEGV`.
   - Always output builds to separate artifacts or unlink old files (`rm -f file.so && cp new.so file.so`).

3. **Never create or write project files through shell commands**:
   - Do NOT use `cat << 'EOF'`, `echo >`, `tee`, or inline shell redirection to write source code or documentation files.
   - Always use dedicated file creation and editing tools (`write_to_file` and `replace_file_content`).
