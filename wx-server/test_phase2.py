#!/usr/bin/env python3
"""TinyWeChat Phase 2 验收测试：注册 + 好友系统"""

import socket, struct, json, time, sys

INVALID_REQUEST = 200
FRIEND_ALREADY = 311

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
        self.seq += 1
        return self.seq

    def login(self, username, password):
        send_frame(self.sock, "auth.login", {"username": username, "password": password}, self.next_seq())
        ft, resp = recv_frame(self.sock)
        assert ft == 0x02 and resp["body"].get("code") == 0, f"Login failed: {resp}"
        u = resp["body"]
        print(f"  [{self.name}] 登录: uid={u['user_id']} {u['nickname']}")
        return u

    def register(self, username, password, nickname=""):
        send_frame(self.sock, "auth.register",
                   {"username": username, "password": password, "nickname": nickname or username},
                   self.next_seq())
        ft, resp = recv_frame(self.sock)
        assert ft == 0x02, f"Register response not RESPONSE: {resp}"
        return resp["body"]

    def req(self, msg_type, body):
        send_frame(self.sock, msg_type, body, self.next_seq())
        ft, resp = recv_frame(self.sock)
        return resp

    def recv_notification(self, timeout=5):
        ft, data = recv_frame(self.sock, timeout)
        assert ft == 0x03, f"Expected NOTIFICATION, got {ft}: {data}"
        return data

    def close(self):
        self.sock.close()


print("=== TinyWeChat Phase 2 验收测试 ===\n")

print("0) 注册新用户 dave ...")
c = Client("RegClient")
r = c.register("dave", "123456", "Dave")
assert r["code"] == 0, f"Register failed: {r}"
print(f"   PASS: dave uid={r['user_id']}")

r = c.register("dave", "123456")
assert r["code"] == 400, f"Expected 400 DUPLICATE_USERNAME, got {r}"
print("   PASS: 重复注册 → code=400")
c.close()

print("\n1) Alice / Bob 登录...")
alice = Client("Alice"); alice.login("alice", "123456")
bob   = Client("Bob");   bob.login("bob", "123456")

print("\n2) Alice → Bob 好友申请...")
resp = alice.req("friend.request", {"to_user_id": 2, "message": "加个好友吧"})
assert resp["body"]["code"] == 0, f"Request failed: {resp}"
print("   PASS: 好友申请已发送")

print("\n3) Bob 收到 friend.notify...")
notif = bob.recv_notification()
assert notif["type"] == "friend.notify"
body = notif["body"]
assert body["from_user_id"] == 1
req_id = body["request_id"]
print(f"   PASS: from {body['from_nickname']} request_id={req_id}")

print("\n4) Bob 查询待处理申请...")
resp = bob.req("friend.pending", {})
pending = resp["body"]["requests"]
assert len(pending) == 1
print(f"   PASS: {len(pending)} 条待处理 (from {pending[0]['from_nickname']})")

print("\n5) Bob 同意申请...")
resp = bob.req("friend.handle", {"request_id": req_id, "action": "accept"})
assert resp["body"]["code"] == 0
print("   PASS: 已同意")

print("\n6) 检查双方好友列表...")
resp = alice.req("friend.list", {})
assert len(resp["body"]["friends"]) == 1
print(f"   Alice: {resp['body']['friends']}")
resp = bob.req("friend.list", {})
assert len(resp["body"]["friends"]) == 1
print(f"   Bob: {resp['body']['friends']}")

print("\n7) Edge: 向自己发申请...")
resp = alice.req("friend.request", {"to_user_id": 1, "message": "hi"})
assert resp["body"]["code"] == INVALID_REQUEST
print(f"   PASS: code={INVALID_REQUEST}")

print("\n8) Edge: 已是好友...")
resp = alice.req("friend.request", {"to_user_id": 2, "message": "again"})
assert resp["body"]["code"] == FRIEND_ALREADY
print(f"   PASS: code={FRIEND_ALREADY}")

alice.close(); bob.close()
print("\n=== Phase 2 全部测试通过! ===")
