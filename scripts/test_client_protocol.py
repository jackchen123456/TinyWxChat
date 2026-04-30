#!/usr/bin/env python3
"""验证客户端协议层与 TinyWeChat 服务端的交互"""
import socket
import struct
import json
import time
import sys

HOST = "127.0.0.1"
PORT = 9090

def build_frame(msg_type, body, seq=1, frame_type=0x01):
    payload = json.dumps({
        "type": msg_type,
        "seq": seq,
        "body": body
    }).encode("utf-8")
    header = struct.pack("!IHBB", len(payload), 0x0001, frame_type, 0x00)
    return header + payload

def send_frame(sock, msg_type, body, seq=1, frame_type=0x01):
    sock.sendall(build_frame(msg_type, body, seq, frame_type))
    print(f"  -> {msg_type}: {json.dumps(body, ensure_ascii=False)[:80]}")

def recv_frame(sock):
    """读取一帧，返回 (frame_type, 完整 payload dict, body dict)"""
    header = b""
    while len(header) < 8:
        chunk = sock.recv(8 - len(header))
        if not chunk:
            return None, None, None
        header += chunk
    length, version, frame_type, flags = struct.unpack("!IHBB", header)
    payload_data = b""
    while len(payload_data) < length:
        chunk = sock.recv(length - len(payload_data))
        if not chunk:
            return None, None, None
        payload_data += chunk
    obj = json.loads(payload_data.decode("utf-8"))
    body = obj.get("body", obj)
    return frame_type, obj, body

def test_register(sock, username, password, nickname):
    send_frame(sock, "auth.register", {
        "username": username, "password": password, "nickname": nickname
    }, seq=1)
    ft, _, body = recv_frame(sock)
    ok = body.get("code") == 0
    print(f"  <- register_res: ok={ok}, {json.dumps(body, ensure_ascii=False)}")
    return ok

def test_login(sock, username, password):
    send_frame(sock, "auth.login", {
        "username": username, "password": password
    }, seq=2)
    ft, _, body = recv_frame(sock)
    ok = body.get("code") == 0
    print(f"  <- login_res: ok={ok}, {json.dumps(body, ensure_ascii=False)}")
    return ok, body

def test_send(sock, seq, to_user_id, content):
    send_frame(sock, "chat.send", {
        "to_user_id": to_user_id, "content": content, "msg_type": 1
    }, seq=seq)
    ft, _, body = recv_frame(sock)
    ok = body.get("code") == 0
    print(f"  <- chat.send_res: ok={ok}, msg_id={body.get('msg_id')}")
    return ok

def test_history(sock, seq, with_user_id):
    send_frame(sock, "chat.history", {
        "with_user_id": with_user_id, "before_msg_id": 0, "limit": 20
    }, seq=seq)
    ft, _, body = recv_frame(sock)
    msgs = body.get("messages", [])
    print(f"  <- history_res: ok={body.get('code')==0}, msgs={len(msgs)}")
    return body

def main():
    print("=== TinyWeChat 客户端协议验证 ===")
    print(f"连接 {HOST}:{PORT}...")

    # ── Alice ──
    s1 = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    try:
        s1.connect((HOST, PORT))
        print("TCP 连接成功 (Alice)")
    except Exception as e:
        print(f"连接失败: {e}")
        sys.exit(1)

    print("\n[1] 尝试注册 alice...")
    if not test_register(s1, "alice", "123456", "Alice"):
        print("  (注册失败，可能已存在)")
        s1.close()
        s1 = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        s1.connect((HOST, PORT))

    print("\n[2] 登录 alice...")
    ok, resp = test_login(s1, "alice", "123456")
    if not ok:
        print("登录失败！检查密码是否为 123456")
        s1.close()
        sys.exit(1)
    alice_id = resp.get("user_id")
    print(f"  Alice ID = {alice_id}")

    # ── Bob ──
    s2 = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    s2.connect((HOST, PORT))
    print("\n[3] 尝试注册 bob...")
    if not test_register(s2, "bob", "123456", "Bob"):
        s2.close()
        s2 = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        s2.connect((HOST, PORT))

    print("\n[4] 登录 bob...")
    ok, resp = test_login(s2, "bob", "123456")
    if not ok:
        print("Bob 登录失败！")
        s2.close()
        s1.close()
        sys.exit(1)
    bob_id = resp.get("user_id")
    print(f"  Bob ID = {bob_id}")

    # ── 发送消息 ──
    seq_a = 10
    print(f"\n[5] Alice -> Bob: '你好 Bob！'")
    test_send(s1, seq_a, bob_id, "你好 Bob！")
    seq_a += 1

    # Bob 收推送
    print("\n[6] Bob 收推送...")
    s2.settimeout(1.5)
    try:
        ft, obj, body = recv_frame(s2)
        if body:
            print(f"  <- recv: from={body.get('from_nickname')}, content=\"{body.get('content')}\"")
        else:
            print("  (未收到)")
    except socket.timeout:
        print("  (超时，服务端可能未推送)")

    # Bob 拉历史
    print("\n[7] Bob 拉取与 Alice 的历史...")
    s2.settimeout(3.0)
    test_history(s2, 20, alice_id)

    # Bob -> Alice
    print(f"\n[8] Bob -> Alice: '你好 Alice！'")
    test_send(s2, 21, alice_id, "你好 Alice！")

    s1.settimeout(1.5)
    try:
        ft, obj, body = recv_frame(s1)
        if body:
            print(f"  <- Alice recv: from={body.get('from_nickname')}, content=\"{body.get('content')}\"")
    except socket.timeout:
        print("  (超时)")

    # Alice 拉历史
    print("\n[9] Alice 拉取与 Bob 的历史...")
    s1.settimeout(3.0)
    test_history(s1, 11, bob_id)

    # 心跳
    print("\n[10] 心跳 PING/PONG...")
    s1.settimeout(3.0)
    send_frame(s1, "ping", {}, seq=99, frame_type=0x04)
    try:
        ft, _, _ = recv_frame(s1)
        print(f"  <- pong OK (frame_type={ft})")
    except:
        print("  pong 超时")

    s1.close()
    s2.close()
    print("\n=== 全部测试完成 ===")

if __name__ == "__main__":
    main()
