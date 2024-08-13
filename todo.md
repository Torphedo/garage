- Resize vehicle after every move which changes the centerpoint. We want to
  keep the vehicle in the center as much as possible.
- Add a button to save the vehicle to a file (trivial)
- Support saving vehicle to an STFS file for use on the console
- Add a new bitmask at 6 bits per cell for part connectivity. Each of the 6
  bits indicate if one of its sides is connectable.

- Look into controller support via GLFW. I think D-pads are exposed as "hat"
  switches, the triggers are one-axis "joysticks", and the actual sticks are
  2-axis joysticks.
- Consider making a 6-axis snapping camera view like the game
- Try to improve ease of depth perception (maybe some shadows?)

# Selection
Selection is a mess. The way it's set up right now can only work with our
selection flag, which is hard to not overlap with real data. Right now these
are the obstacles to removing the selection flag:
- The only way to check if a part is selected is to check all its cells against
  the selection grid. If a selected part overlaps a non-selected one, both of
  them appear selected. If all their cells are both occupied and selected, the
  parts are completely indistinguishable. As far as I can tell, this problem
  is unsolvable using just grids.

Some ideas to solve the issue:
- Keep 2 lists of parts, one for selected and one for unselected. If we want to
  avoid copying around the part data, they could just be arrays of indices into
  the part array. For the reasons mentioned, we can't rely on the grid at all
  to tell if something is selected. The only way to find out selection is to do
  a part_by_pos() check, then search for the index in the selected list. We
  should remove selected parts from occupancy and delete the selection mask
  completely.
- We don't need the unselected list, doing just a selected list is easier

Other tweaks:
- In scenarios where we move the entire bitmask, we could implement it with
  bitshifts & memcpy()
- When a part is selected, just delete all its cells from occupancy. Why are we
  clearing the entire grid??

