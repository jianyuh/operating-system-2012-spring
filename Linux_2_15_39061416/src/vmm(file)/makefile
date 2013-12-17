success	:request response

response	:vmm_response.c vmm_global.h vmm_pagereplace.h
	cc -o response vmm_response.c
request		:vmm_request.c vmm_global.h
	cc -o request vmm_request.c
clean:
	rm request response
