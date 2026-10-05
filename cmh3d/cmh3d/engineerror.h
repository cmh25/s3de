#ifndef ENGINEERROR_H
#define ENGINEERROR_H

#ifdef __cplusplus
extern "C" {
#endif

/* The engine never exits the process. A call that fails returns 0 or NULL and
   leaves a message here. InitializeWorld() and DrawScene() clear the message
   on entry, so after a successful InitializeWorld() a non-empty message is a
   warning, for example a texture that could not be loaded. */
const char* GetLastEngineError(void);

/* for the engine's own use */
void SetEngineError(const char* fmt, ...);
void ClearEngineError(void);

#ifdef __cplusplus
}
#endif

#endif /* ENGINEERROR_H */
