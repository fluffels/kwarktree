// TODO: this is wrong because it assumes a positive Y going "up"
vec3 rotorRotate(vec4 p, vec3 x) {
    vec3 q;
	q.x = p.w * x.x + x.y * p.x + x.z * p.y;
	q.y = p.w * x.y - x.x * p.x + x.z * p.z;
	q.z = p.w * x.z - x.x * p.y - x.y * p.z;

	float q012 = x.x * p.z - x.y * p.y + x.z * p.x; // trivector

	// r = q P*
	vec3 r;
	r.x = p.w * q.x + q.y  * p.x + q.z  * p.y    + q012 * p.z;
	r.y = p.w * q.y - q.x  * p.x - q012 * p.y    + q.z  * p.z;
	r.z = p.w * q.z + q012 * p.x - q.x  * p.y    - q.y  * p.z;

    return r;
}
