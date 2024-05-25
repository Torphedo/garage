- Don't allow part to be placed overlapping another (use a new function for
  this check, so we can account for large & weird-shaped parts later)
- Allow selecting multiple parts at once (maybe by re-using padding bytes as a
  selection flag, or keeping our own list). Trying to select an already selected
  part should let you start moving all those selected, like in the game.
- Highlight selected parts in some way
- Resize vehicle after every move which changes the centerpoint. We want to
  keep the vehicle in the center as much as possible.
- Add a button to save the vehicle to a file (trivial)
- Support saving vehicle to an STFS file for use on the console

- Look into controller support via GLFW. I think D-pads are exposed as "hat"
  switches, the triggers are one-axis "joysticks", and the actual sticks are
  2-axis joysticks.
- Consider making a 6-axis snapping camera view like the game
- Try to improve ease of depth perception (maybe some shadows?)
- Research the file format more to figure out if parts are stored in a heap
  structure or any sort of predictable order that could speed up searches. I've
  noticed the game will change the order of some parts while saving, even when 
  it seems unnecessary...

