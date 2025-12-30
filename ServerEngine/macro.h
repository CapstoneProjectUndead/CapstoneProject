#define CRASH(cause)						\
{											\
	uint32* crash = nullptr;				\
	__analysis_assume(crash != nullptr);	\
	*crash = 0xDEADBEEF;					\
}

#define ASSERT_CRASH(expr)			\
{									\
	if (!(expr))					\
	{								\
		CRASH("ASSERT_CRASH");		\
		__analysis_assume(expr);	\
	}								\
}

#define COPY_STRING(dest, src)  memset(dest, 0, sizeof(dest)); memcpy(dest, src, strlen(src));

#define MAKE_SEND_BUFFER(pkt) 	SendBufferRef sendBuffer = make_shared<SendBuffer>(pkt.GetSize());  \
								sendBuffer->CopyData(&pkt, pkt.GetSize());							\
								sendBuffer->Close(pkt.GetSize());