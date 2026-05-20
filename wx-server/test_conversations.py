#!/usr/bin/env python3
"""验证新增接口：chat.conversations + group.apply_notify"""

import socket, struct, json, time

HOST, PORT = "127.0.0.1", 19090

def build_frame(msg_type, body, seq=1, frame_type=0x01):
    payload = json.dumps({"type": msg_type, "seq": seq, "body": body}).encode()
    return struct.pack("!IHBB", len(payload), 0x0001, frame_type, 0x00) + payload

def send_frame(sock, msg_type, body, seq=1, frame_type=0x01):
    sock.sendall(build_frame(msg_type, body, seq, frame_type))

def recv_frame(sock, timeout=5):
    sock.settimeout(timeout)
    header = b""
    while len(header) < 8:
        c = sock.recv(8 - len(header))
        if not c: return None, None
        header += c
    length, _, frame_type, _ = struct.unpack("!IHBB", header)
    payload = b""
    while len(payload) < length:
        c = sock.recv(length - len(payload))
        if not c: return None, None
        payload += c
    return frame_type, json.loads(payload) if payload else {}

class Client:
    def __init__(self, name):
        self.name = name
        self.sock = socket.create_connection((HOST, PORT), timeout=5)
        self.seq = 0
    def next_seq(self):
        self.seq += 1; return self.seq
    def login(self, u, p):
        send_frame(self.sock, "auth.login", {"username": u, "password": p}, self.next_seq())
        ft, resp = recv_frame(self.sock)
        assert ft == 0x02 and resp["body"]["code"] == 0
        return resp["body"]
    def req(self, t, b):
        send_frame(self.sock, t, b, self.next_seq())
        ft, resp = recv_frame(self.sock)
        return resp
    def recv_notif(self, timeout=5):
        ft, data = recv_frame(self.sock, timeout)
        assert ft == 0x03, f"Expected NOTIFICATION got {ft}: {data}"
        return data
    def close(self): self.sock.close()


print("=== chat.conversations 接口测试 ===\n")

alice = Client("Alice"); alice.login("alice", "123456")
bob   = Client("Bob");   bob.login("bob", "123456")
carol = Client("Carol"); carol.login("carol", "123456")

# Alice sends to Bob
alice.req("chat.send", {"to_user_id": 2, "content": "Hi Bob", "msg_type": 1})
bob.recv_notif(timeout=2)

# Alice sends to Carol
alice.req("chat.send", {"to_user_id": 3, "content": "Hi Carol", "msg_type": 1})
carol.recv_notif(timeout=2)

# Bob replies
bob.req("chat.send", {"to_user_id": 1, "content": "Hey Alice", "msg_type": 1})
alice.recv_notif(timeout=2)

print("1) Alice 查询会话列表...")
resp = alice.req("chat.conversations", {})
assert resp["body"]["code"] == 0
convs = resp["body"]["conversations"]
assert len(convs) == 2, f"Expected 2 conversations, got {len(convs)}"
print(f"   PASS: {len(convs)} 个会话")
for c in convs:
    print(f"   {c['peer_nickname']}: {c['last_content']} (ts={c['last_timestamp']})")

print("\n2) Bob 查询会话列表...")
resp = bob.req("chat.conversations", {})
convs = resp["body"]["conversations"]
assert len(convs) == 1
assert convs[0]["peer_nickname"] == "Alice"
assert convs[0]["last_content"] == "Hey Alice"
print(f"   PASS: 会话 {convs[0]['peer_nickname']}: {convs[0]['last_content']}")

print("\n3) Carol 查询会话列表...")
resp = carol.req("chat.conversations", {})
convs = resp["body"]["conversations"]
assert len(convs) == 1
print(f"   PASS: 会话 {convs[0]['peer_nickname']}: {convs[0]['last_content']}")

print("\n4) Dave（无会话）查询...")
dave = Client("Dave"); dave.login("dave", "123456")
resp = dave.req("chat.conversations", {})
convs = resp["body"]["conversations"]
assert len(convs) == 0
print("   PASS: 无会话（空列表）")
dave.close()

# Cleanup
alice.close(); bob.close(); carol.close()

print("\n=== chat.conversations 测试全部通过! ===")
