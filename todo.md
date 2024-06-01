- Resize vehicle after every move which changes the centerpoint. We want to
  keep the vehicle in the center as much as possible.
- Add a button to save the vehicle to a file (trivial)
- Support saving vehicle to an STFS file for use on the console
- Load models from OBJ. We'll either have to do 2 passes on the file (one to
  find out the size and one to fill out the model struct), or grab the queue
  implementation from koshka to use a dynamic list. OBJ doesn't support vertex
  colors, but Blender and other tools will export it by giving each vertex 6
  floats (x, y, z, r, g, b). https://paulbourke.net/dataformats/obj/colour.html
- Make a single-part volume bitmask to check collisions. The largest part is
  the Large Box (7x8x2) and we'll need to be able to rotate the part, so the
  volume needs to be 8x8x8 (64 bytes).

- Look into controller support via GLFW. I think D-pads are exposed as "hat"
  switches, the triggers are one-axis "joysticks", and the actual sticks are
  2-axis joysticks.
- Consider making a 6-axis snapping camera view like the game
- Try to improve ease of depth perception (maybe some shadows?)
- Research the file format more to figure out if parts are stored in a heap
  structure or any sort of predictable order that could speed up searches. I've
  noticed the game will change the order of some parts while saving, even when 
  it seems unnecessary...

