# Glossy
A rudimentary and simplistic pathtracer written in GLSL. It runs entirely inside the fragment shader.

# Screenshot
![Screenshot](https://raw.githubusercontent.com/Fytch/Glossy/master/pictures/screenshot.png)

Make sure to check out the examples in the [scenes/](scenes/) subdirectory. To launch one, simply enter `./Glossy ./scenes/three_lights.json`.

# Features
- All calculations are done on the graphics cards
- Configurable scene files in JSON
- Diffuse and specular lighting
- Recursive pathtracing

# *Where are the .frag files?*
There are none. The GLSL fragment shader code is actually being generated at run-time based on the given JSON scene file. Check out [json2glsl.cpp](src/json2glsl.cpp) if you're interested.

# Dependencies
- [SFML](https://github.com/SFML/SFML) for the OS dependent window and OpenGL code.
- [nlohmann/json](https://github.com/nlohmann/json) for parsing JSON files.

# License
*Glossy* is licensed under the [MIT License](https://tldrlegal.com/license/mit-license). See the enclosed [LICENSE.txt](LICENSE.txt) for more information.
