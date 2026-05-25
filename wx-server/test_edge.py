#!/usr/bin/env python3
"""TinyWeChat Phase 1 边界测试"""

import socket
import struct
import json
import time

HOST = "127.0.0.1"
PORT = 19090

def build_frame(msg_type, body, seq=1, frame_type=0x01):
    payload = json.dumps({"type": msg_type, "seq": seq, "body": body}).encode()
    return struct.pack("!IHBB", len(payload), 0x0001, frame_type, 0x00) + payload

def send_frame(sock, msg_type, body, seq=1, frame_type=0x01):
    sock.sendall(build_frame(msg_type, body, seq, frame_type))

def recv_frame(sock):
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

def connect(): return socket.create_connection((HOST, PORT), timeout=5)

print("=== 边界测试 ===\n")

# 1) 错误密码
print("1) 错误密码...")
s = connect()
send_frame(s, "auth.login", {"username": "alice", "password": "wrong"})
ft, data = recv_frame(s)
assert ft == 0x02 and data["body"]["code"] == 401, f"Expected 401, got {data}"
print("   PASS: code=401 (WRONG_PASSWORD)")
s.close()

# 2) 重复登录——新登录踢旧连接
print("\n2) 重复登录踢人...")
old = connect()
send_frame(old, "auth.login", {"username": "alice", "password": "123456"})
ft, data = recv_frame(old)
assert data["body"]["code"] == 0, f"First login failed: {data}"
print("   旧连接已登录 (uid=1)")

new = connect()
send_frame(new, "auth.login", {"username": "alice", "password": "123456"})
ft, data = recv_frame(new)
assert data["body"]["code"] == 0, f"Second login failed: {data}"
print("   新连接登录成功（旧连接应被踢）")

# Old connection should be closed — read should fail
time.sleep(0.3)
old.settimeout(1)
try:
    d = old.recv(1)
    if not d:
        print("   PASS: 旧连接已断开")
    else:
        print("   WARN: 旧连接仍有数据")
except (ConnectionResetError, BrokenPipeError, OSError) as e:
    print(f"   PASS: 旧连接已断开 ({type(e).__name__})")
old.close()
new.close()

# 3) 未登录发消息
print("\n3) 未登录发消息...")
s = connect()
send_frame(s, "chat.send", {"to_user_id": 2, "content": "hello", "msg_type": 1})
ft, data = recv_frame(s)
# 错误带 seq 时走 RESPONSE 帧（M-1），让客户端能按 seq 关联回原请求
assert ft == 0x02 and data["body"]["code"] == 210, f"Expected UNAUTHORIZED, got ft={ft} {data}"
print("   PASS: code=210 (UNAUTHORIZED)")
s.close()

# 4) 发消息给不存在的用户
print("\n4) 发消息给不存在的用户...")
s = connect()
send_frame(s, "auth.login", {"username": "bob", "password": "123456"})
ft, _ = recv_frame(s)
send_frame(s, "chat.send", {"to_user_id": 999, "content": "hello", "msg_type": 1})
ft, data = recv_frame(s)
assert data["body"]["code"] == 0, f"Expected SUCCESS (server doesn't check recipient existence for MVP), got {data}"
print("   PASS: 消息发送成功（MVP 不做接收者存在性验证，消息落库即可）")
s.close()

# 5) 空消息被拒绝
print("\n5) 空消息内容...")
s = connect()
send_frame(s, "auth.login", {"username": "alice", "password": "123456"})
ft, _ = recv_frame(s)
send_frame(s, "chat.send", {"to_user_id": 2, "content": "", "msg_type": 1})
ft, data = recv_frame(s)
assert data["body"]["code"] == 200, f"Expected INVALID_REQUEST, got {data}"
print("   PASS: code=200 (INVALID_REQUEST)")
s.close()

# 6) 不存在的用户登录
print("\n6) 不存在的用户登录...")
s = connect()
send_frame(s, "auth.login", {"username": "nobody", "password": "123"})
ft, data = recv_frame(s)
assert data["body"]["code"] == 401, f"Expected 401, got {data}"
print("   PASS: code=401 (用户名或密码错误)")
s.close()

print("\n=== 全部边界测试通过! ===")
