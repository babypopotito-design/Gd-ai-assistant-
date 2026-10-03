# GD AI Assistant

A Geode mod prototype that adds an AI Assistant button to the Geometry Dash level editor.

## Current milestone

- Adds an assistant button to `LevelEditorLayer`.
- Opens a chat-style popup.
- Accepts a natural-language instruction.
- Runs in safe preview mode and logs the instruction.
- Does **not** embed an API key or send user data anywhere yet.

## Build

Install the Geode CLI and SDK for the platform you target, then run from this directory:

```sh
geode build
```

The resulting `.geode` package can be installed through the Geode launcher or copied into the Geode mods directory.

## Next milestone: real AI relay

The mod should send requests only to a user-configured HTTPS relay. The relay keeps credentials off the user's game client and should accept a prompt plus a structured command schema, for example:

```json
{
  "prompt": "make a simple jump section",
  "context": { "levelName": "My Level", "editorVersion": "2.2" }
}
```

The response should be validated before any editor objects are created:

```json
{
  "actions": [
    { "type": "place", "objectId": 1, "x": 300, "y": 150, "rotation": 0 }
  ]
}
```

Do not allow arbitrary code execution, and require an explicit **Apply** action before changing a level.

## Notes

This sandbox does not have the Geode CLI or Geometry Dash SDK installed, so the source has not been compiled here. Verify the current Geode SDK class signatures on the target platform before building.

## One-command build

After installing and configuring Geode on the target computer:

```sh
./build.sh
```

For a Windows build from Linux, use the Geode platform option if the cross-toolchain is installed:

```sh
./build.sh -p windows
```

## Android phone build

This project is source-compatible with Geode Android. Build an ARM64 package with:

```sh
./build-android64.sh
```

Copy the resulting `.geode` file to:

```text
Android/media/com.geode.launcher/game/geode/mods/
```

Then restart Geometry Dash through the Geode Android Launcher and enable the mod in the Geode menu. If the phone is an older 32-bit Android device, use `geode build -p android32` instead.
