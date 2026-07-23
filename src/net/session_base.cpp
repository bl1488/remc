#include "session_base.h"
#include "server.h"
#include "client.h"

namespace remc::net {

// Instance here
template class SessionBase<SessionServer>;
template class SessionBase<SessionClient>;

} // namespace remc::net
