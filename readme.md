# Garage Opener
This is a simplified vehicle editor for Banjo-Kazooie: Nuts & Bolts, without
the game's arbitrary restrictions. The only restrictions in this editor are
ones that stop you from creating an invalid/unusable vehicle. For example, the
vehicle file format has a size limit of 127 in all 3 directions, and allowing
parts to overlap causes one to be deleted when loading.

## Usage
Garage Opener supports the save file container used by the Xbox, and the "raw"
vehicle files saved by Xenia. Because of the console's `STFS` container,
console files take up a bit more space than Xenia files (60-70KiB vs. 0-16KiB).

<details>
<summary> Click this dropdown for Xbox 360 instructions</summary>
Using Garage Opener does not require modding your Xbox 360! From the console,
you can copy your saved vehicles to a thumb drive or USB hard drive and open
them on PC. Use a blank drive with nothing on it, formatted as FAT32 (you'll
have to reformat it on the console, but this makes sure the console will see
it).

- Sign in to the account you want to copy vehicles from
- On the Home menu, navigate to `Settings > System > Storage`
- Plug in the empty USB drive and format it so the console can use it
- Go to `Hard Drive > Games and Apps > Banjo Kazooie: N&B`
- Scroll past all your replays, photos, and `BANJO SAVE GAME` entries. (use the
  triggers to move a page at a time). Each vehicle is treated as a separate
  save file.
- For each vehicle, click on it & copy it to `USB Storage Device`
- Plug the USB drive into your PC. Your vehicle files will be in:     
  `Content/[Xbox profile ID number]/4D5307ED/00000001`
</details>

<details>
<summary> Click this dropdown for Xenia instructions</summary>
If you use Xenia with all default settings, your Xenia folder is
`Documents\xenia` (you can also jump there by hitting Ctrl-L in File Explorer,
pasting `%USERPROFILE%\Documents\xenia` and hitting Enter). If you use Xenia in
portable mode, your Xenia folder is the folder with `xenia.exe`, or whatever
paths you set for `content_root` in your `xenia.config.toml`.

In your Xenia folder, N&B save files are in `content/4D5307ED/00000001`.
The `0x0b0a5c5c` and `0x0b0d6cca` folders store your progress, and every other
folder contains a vehicle.
</details>

Once you've found your vehicle files, drag-and-drop any of them onto Garage
Opener to open them. The console's STFS container will be handled automatically
(assuming your vehicle is < 680KiB, which it should be).

