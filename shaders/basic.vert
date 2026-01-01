    #version 330 core
    layout (location = 0) in vec2 aPos;
    layout (location = 1) in vec4 aColor;
    layout (location = 2) in vec2 aTexCoord;
    layout (location = 3) in float aType;

    out vec4 vColor;
    out vec2 vTexCoord;
    out float vType;

    uniform mat4 uProjection;
    uniform mat4 uView;

    void main() {
       vColor = aColor;
       vTexCoord = aTexCoord;
       vType = aType;
       gl_Position = uProjection * uView * vec4(aPos.x, aPos.y, 0.0, 1.0);
    }