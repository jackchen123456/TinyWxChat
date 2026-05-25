#include "protocol.h"
#include <cstring>
#include <arpa/inet.h>   // htonl, htons, ntohl, ntohs

// ── Header encode / decode ───────────────────────────────

void buildHeader(uint8_t* buf, uint32_t length, FrameType type,
                 uint8_t flags, uint16_t version)
{
    uint32_t netLen = htonl(length);
    uint16_t netVer = htons(version);
    std::memcpy(buf + 0, &netLen, 4);
    std::memcpy(buf + 4, &netVer, 2);
    buf[6] = static_cast<uint8_t>(type);
    buf[7] = flags;
}

bool parseHeader(const uint8_t* buf, Frame& out)
{
    uint32_t netLen;
    uint16_t netVer;
    std::memcpy(&netLen, buf + 0, 4);
    std::memcpy(&netVer, buf + 4, 2);

    out.length  = ntohl(netLen);
    out.version = ntohs(netVer);
    out.frameType = static_cast<FrameType>(buf[6]);
    out.flags      = buf[7];

    if (out.version != PROTOCOL_VERSION) {
        return false;
    }
    if (out.length > MAX_PAYLOAD_SIZE)
        return false;

    // validate frame_type range
    uint8_t ft = buf[6];
    if (ft < 1 || ft > 4)
        return false;

    return true;
}

std::vector<uint8_t> serializeFrame(const Frame& frame)
{
    std::vector<uint8_t> buf(FRAME_HEADER_SIZE + frame.payload.size());
    buildHeader(buf.data(), static_cast<uint32_t>(frame.payload.size()),
                frame.frameType, frame.flags, frame.version);
    if (!frame.payload.empty())
        std::memcpy(buf.data() + FRAME_HEADER_SIZE,
                    frame.payload.data(), frame.payload.size());
    return buf;
}

// ── Message builders ────────────────────────────────────

Frame buildResponse(const std::string& type, int seq, const json& body)
{
    json j;
    j["type"] = type;
    j["seq"]  = seq;
    j["body"] = body;

    Frame f;
    f.frameType = FrameType::RESPONSE;
    f.payload   = j.dump();
    return f;
}

Frame buildNotification(const std::string& type, const json& body)
{
    json j;
    j["type"] = type;
    j["seq"]  = 0;    // notifications don't need a seq
    j["body"] = body;

    Frame f;
    f.frameType = FrameType::NOTIFICATION;
    f.payload   = j.dump();
    return f;
}

Frame buildError(int code, const std::string& msg, int seq)
{
    json body;
    body["code"]    = code;
    body["message"] = msg;

    json j;
    j["type"] = MsgType::ERROR;
    j["seq"]  = seq;
    j["body"] = body;

    Frame f;
    // When seq > 0 this is an error RESPONSE to a specific request;
    // otherwise it is an unsolicited NOTIFICATION.
    f.frameType = (seq > 0) ? FrameType::RESPONSE : FrameType::NOTIFICATION;
    f.payload   = j.dump();
    return f;
}
