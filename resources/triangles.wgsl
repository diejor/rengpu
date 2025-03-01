
// @group(0) @binding(0) var<uniform> uTime: f32;

struct VertexInput {
    @location(0) position: vec2f,
    @location(1) color: vec3f,
};

struct VertexOutput {
    @builtin(position) position: vec4f,
    @location(0) color: vec3f,
};

@vertex
fn vs_main(in: VertexInput) -> VertexOutput {
    var out: VertexOutput;
    let ratio = 800.0 / 600.0; // The width and height of the target surface

    //var offset = 0.3 * vec2f(cos(uTime), sin(uTime));


    out.position = vec4f(in.position.x, in.position.y*ratio, 0.0, 1.0);
    out.color = in.color;
    return out;
}

@fragment
fn fs_main(in: VertexOutput) -> @location(0) vec4f {
    // We apply a gamma-correction to the color
    // We need to convert our input sRGB color into linear before the target
    // surface converts it back to sRGB.
    let linear_color = pow(in.color, vec3f(2.2));
    return vec4f(linear_color, 1.0);
}
