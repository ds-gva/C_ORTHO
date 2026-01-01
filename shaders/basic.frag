    #version 330 core
    out vec4 FragColor;
    
    in vec4 vColor;
    in vec2 vTexCoord;
    in float vType; 

    uniform sampler2D uTexture;
    
    void main() {
       vec4 texColor = texture(uTexture, vTexCoord);
       vec4 finalColor = texColor * vColor;
       float alpha = finalColor.a;

    // --- TYPE 1: SOLID CIRCLE ---\n"
    if (vType > 0.9 && vType < 1.1) {
        float dist = distance(vTexCoord, vec2(0.5));
        float delta = 0.01;
        alpha = smoothstep(0.5, 0.5 - delta, dist);
    }
    
    // --- TYPE 2: HOLLOW CIRCLE (Border) ---\n"
    else if (vType > 1.9 && vType < 2.1) {
        float dist = distance(vTexCoord, vec2(0.5));
        float delta = 0.01;
        float border = 0.05; // Thickness
        // Cut out the middle
        float outer = smoothstep(0.5, 0.5 - delta, dist);
        float inner = smoothstep(0.5 - border, 0.5 - border - delta, dist);
        alpha = outer - inner;
    }

    // --- TYPE 3: HOLLOW RECT (Border) ---\n"
    else if (vType > 2.9 && vType < 3.1) {
        float border = 0.05; // Thickness
        // Check if we are close to any edge (u=0, u=1, v=0, v=1)
        float mask = 0.0;
        if (vTexCoord.x < border || vTexCoord.x > 1.0 - border) mask = 1.0;
        if (vTexCoord.y < border || vTexCoord.y > 1.0 - border) mask = 1.0;
        alpha = mask;
    }

    finalColor.a = alpha * vColor.a;
    FragColor = finalColor;
    if (FragColor.a < 0.01) discard;
    }