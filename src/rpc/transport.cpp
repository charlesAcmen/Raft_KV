#include "rpc/transport.h"
namespace rpc{
ITransport::ITransport(
    const type::PeerInfo& self,
    const std::vector<type::PeerInfo>& peers)
    : self_(self), peers_(peers) {}
}// namespace rpc