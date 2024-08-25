- Resize vehicle after every move which changes the centerpoint. We want to
  keep the vehicle in the center as much as possible.
- Add a button to save the vehicle to a file (trivial)
- Support saving vehicle to an STFS file for use on the console
- Add a new bitmask at 6 bits per cell for part connectivity. Each of the 6
  bits indicate if one of its sides is connectable.

- Consider making a 6-axis snapping camera view like the game
- Try to improve ease of depth perception (maybe some shadows?)

# Selection
Other tweaks:
- In scenarios where we move the entire bitmask, we could implement it with
  bitshifts or memcpy()
- When a part is selected, just delete all its cells from occupancy. Why are we
  clearing the entire grid??
- This might be overkill, but BVH trees look really cool and this could be a
  decent excuse to use them. We could replace the grid lookup with a BVH tree
  containing smaller grids, reducing memory usage & improving lookups

