#!/usr/bin/env python3
"""TinyWeChat Phase 3 验收测试：群聊系统"""

import socket, struct, json, time, sys

HOST, PORT = "127.0.0.1", 19090
GROUP_ALREADY_MEMBER = 321
GROUP_PERMISSION_DENIED = 322

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

    def req(self, msg_type, body):
        send_frame(self.sock, msg_type, body, self.next_seq())
        ft, resp = recv_frame(self.sock)
        return ft, resp

    def recv_notification(self, timeout=5):
        ft, data = recv_frame(self.sock, timeout)
        assert ft == 0x03, f"Expected NOTIFICATION, got {ft}: {data}"
        return data

    def close(self):
        self.sock.close()


print("=== TinyWeChat Phase 3 验收测试 ===\n")

# ── 1. Login ────────────────────────────────────────────
print("1) Alice / Bob / Carol 登录...")
alice = Client("Alice"); alice.login("alice", "123456")
bob   = Client("Bob");   bob.login("bob", "123456")
carol = Client("Carol"); carol.login("carol", "123456")

# ── 2. Alice creates a group ───────────────────────────
print("\n2) Alice 创建群「周末爬山队」...")
ft, resp = alice.req("group.create", {"name": "周末爬山队"})
assert ft == 0x02 and resp["body"]["code"] == 0, f"Group create failed: {resp}"
group_id = resp["body"]["group_id"]
print(f"   PASS: 群已创建 group_id={group_id}")

# ── 3. Bob joins the group ─────────────────────────────
print("\n3) Bob 加入群...")
ft, resp = bob.req("group.join", {"group_id": group_id})
assert resp["body"]["code"] == 0, f"Join failed: {resp}"
print("   PASS: Bob 已加入")

# ── 4. Carol joins the group ───────────────────────────
print("\n4) Carol 加入群...")
ft, resp = carol.req("group.join", {"group_id": group_id})
assert resp["body"]["code"] == 0, f"Join failed: {resp}"
print("   PASS: Carol 已加入")

# ── 5. Alice sends a group message ─────────────────────
print("\n5) Alice 发群消息「周末早上8点集合！」...")
ft, resp = alice.req("group.send", {"group_id": group_id, "content": "周末早上8点集合！", "msg_type": 1})
assert ft == 0x02
assert resp["body"]["code"] == 0, f"Send failed: {resp}"
msg_seq = resp["body"]["msg_seq"]
print(f"   PASS: 消息已发送 msg_seq={msg_seq}")

# ── 6. Bob + Carol receive broadcast ───────────────────
print("\n6) Bob / Carol 收到群消息广播...")
data = bob.recv_notification()
body = data["body"]
assert data["type"] == "group.recv"
assert body["content"] == "周末早上8点集合！"
assert body["from_user_id"] == 1
print(f"   Bob 收到: {body['content']} (from {body['from_nickname']})")

data = carol.recv_notification()
body = data["body"]
assert body["content"] == "周末早上8点集合！"
print(f"   Carol 收到: {body['content']} (from {body['from_nickname']})")

# ── 7. Bob replies ─────────────────────────────────────
print("\n7) Bob 回复「收到！」...")
ft, resp = bob.req("group.send", {"group_id": group_id, "content": "收到！", "msg_type": 1})
assert resp["body"]["code"] == 0

data = alice.recv_notification()
assert data["body"]["content"] == "收到！"
print(f"   Alice 收到: {data['body']['content']}")

data = carol.recv_notification()
assert data["body"]["content"] == "收到！"
print(f"   Carol 收到: {data['body']['content']}")

# ── 8. Group history ───────────────────────────────────
print("\n8) Bob 拉取群历史...")
ft, resp = bob.req("group.history", {"group_id": group_id})
assert resp["body"]["code"] == 0
msgs = resp["body"]["messages"]
assert len(msgs) == 2, f"Expected 2 messages, got {len(msgs)}"
for m in msgs:
    print(f"   msg_seq={m['msg_seq']} {m['from_nickname']}: {m['content']}")

# ── 9. Edge: non-member can't send ─────────────────────
print("\n9) Edge: 非群成员发消息...")
dave = Client("Dave"); dave.login("dave", "123456")
ft, resp = dave.req("group.send", {"group_id": group_id, "content": "hello", "msg_type": 1})
assert resp["body"]["code"] == GROUP_PERMISSION_DENIED
print(f"   PASS: code={GROUP_PERMISSION_DENIED} (GROUP_PERMISSION_DENIED)")

# ── 10. Edge: join already member ──────────────────────
print("\n10) Edge: 重复加入...")
ft, resp = bob.req("group.join", {"group_id": group_id})
assert resp["body"]["code"] == GROUP_ALREADY_MEMBER
print(f"   PASS: code={GROUP_ALREADY_MEMBER} (GROUP_ALREADY_MEMBER)")

alice.close(); bob.close(); carol.close(); dave.close()

print("\n=== Phase 3 全部测试通过! ===")
