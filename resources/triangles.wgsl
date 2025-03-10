
@group(0) @binding(0) var<uniform> u_time: f32;

struct VertexInput {
    @location(0) position: vec3f,
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

    let cosT = cos(u_time);
    let sinT = sin(u_time);
    var position = vec3f(
        in.position.x,
        in.position.y * cosT - in.position.z * sinT,
        in.position.y * sinT + in.position.z * cosT
    );

    out.position = vec4f(position.x, position.y * ratio, position.z*0.5 + 0.5, 1.0);

    out.color = in.color;
    return out;
}

@fragment
fn fs_main(in: VertexOutput) -> @location(0) vec4f {
    // We apply a gamma-correction to the color
    // We need to convert our input sRGB color into linear before the target
    // surface converts it back to sRGB.
    var out_color = vec3f(vec3f(pow(in.position.z, 2.)));
    out_color = in.color;
    let linear_color = pow(out_color, vec3f(2.2));
    return vec4f(linear_color, 1.0);
}
