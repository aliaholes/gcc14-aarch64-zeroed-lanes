f:
	ldr	q31, [x1]
	dup	v30.2s, w2
	add	v31.2s, v30.2s, v31.2s
	str	q31, [x0]
	ret
	.size	f, .-f
