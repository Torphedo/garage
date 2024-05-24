- Don't allow part to be placed overlapping another (check via a function so we
  can account for non-1x1x1 parts later
- Select multiple parts at once (maybe by re-using padding bytes, or keeping
  our own list
- Resize vehicle after every move which changes the centerpoint. We want to
  keep the vehicle in the center as much as possible.
- Try to improve ease of depth perception (maybe some shadows?)
- Save vehicle to file
- STFS write support

- Consider making a 6-axis snapping camera view like the original

