const float amount = 0.05;
const float angle = 1.0;

void mainImage(out vec4 fragColor, in vec2 fragCoord) {
  vec2 vUv = fragCoord.xy / iResolution.xy;
  vec2 offset = amount * vec2(cos(angle * iTime), sin(angle * iTime));
  vec4 cr = texture(iChannel0, vUv + offset);
  vec4 cga = texture(iChannel0, vUv);
  vec4 cb = texture(iChannel0, vUv - offset);
  fragColor = vec4(cr.r, cga.g, cb.b, cga.a);
}
