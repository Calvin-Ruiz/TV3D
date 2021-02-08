>>> Vulkan version <<<
> Build for integraded GPU only for heavy optimizations on transfers <
Escape -> pause
Space, Return -> unpause
Q, close window, termination signal -> Exit
SIZE must match the exact image size in width and height
DUMP is the texture dump path (which must exist)
USEDUMP is on to use dump textures or off to use png textures
DEBUG is on to use debug layers, off to disable them
There is no way to get a sampler2DRect scaled to window size.
Anyway, you may emulate it by dividing texCoord by his expected size when calling texture

With default config.txt, your tree must be :
vulkan
    assets
        source
            source_00
            source_01
            source_02
            source_03
            source_04
            source_05
            source_06
            source_07
    textureDump
        channel0
        channel1
        channel2
        channel3
        channel4
        channel5
        channel6
        channel7

compile.sh may need to be run twice the first time
