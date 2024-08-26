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
- BVH/octree procedure could look like this:
  - Recursively split space into octants until there are 8 parts or less.
  - Lookup now becomes an octree lookup, and at the leaf we just do a linear
    search of the parts in that box.
  - Problem: how do we insert? Perfect for static but moving parts are a pain.
    Just split the leaf volume?
    - If the part(s) move out of their assigned octant(s), remove them from the
      list. Delete the octant(s) if they have no other occupants.
    - Find where the parts should go, and add them to the appropriate octants.
    - If any of the leaves we just added to are over the max parts per octant,
      recursively split the volume.

