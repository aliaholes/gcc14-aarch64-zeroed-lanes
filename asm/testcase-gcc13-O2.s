f:
	ldp	x4, x5, [x1]
	dup	v0.2s, w2
	fmov	d1, x4
	add	v0.2s, v0.2s, v1.2s
	ins	v0.d[1], x5
	str	q0, [x0]
	ret
	.size	f, .-f
