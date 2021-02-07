> Build for integraded GPU only for heavy optimizations on transfers <
Escape -> pause
Space, Return -> unpause
Q, close window, termination signal -> Exit
SIZE must match the exact image size in width and height
DUMP is the texture dump path (which must exist)
USEDUMP is on to use dump textures or off to use png textures
There is no way to get a sampler2DRect scaled to window size.
Anyway, you may emulate it by dividing texCoord by his expected size when calling texture
