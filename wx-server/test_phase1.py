#!/usr/bin/env python3
"""TinyWeChat 服务端 Phase 1 验收脚本
模拟 Alice 和 Bob 两个客户端完成：
  1. 登录
  2. 单聊文本收发
  3. 历史消息拉取
"""

import socket
import struct
import json
import time
import sys

HOST = "127.0.0.1"
PORT = 19090

def build_frame(msg_type, body, seq=1, frame_type=0x01):
    """构建 8 字节头 + JSON payload 的帧"""
    payload = json.dumps({
        "type": msg_type,
        "seq": seq,
        "body": body
    }).encode("utf-8")
    header = struct.pack("!IHBB", len(payload), 0x0001, frame_type, 0x00)
    return header + payload

def send_frame(sock, msg_type, body, seq=1, frame_type=0x01):
    data = build_frame(msg_type, body, seq, frame_type)
    sock.sendall(data)

def recv_frame(sock):
    """读取一帧，返回 (frame_type, dict)"""
    header = b""
    while len(header) < 8:
        chunk = sock.recv(8 - len(header))
        if not chunk:
            return None, None
        header += chunk
    length, version, frame_type, flags = struct.unpack("!IHBB", header)
    payload = b""
    while len(payload) < length:
        chunk = sock.recv(length - len(payload))
        if not chunk:
            return None, None
        payload += chunk
    data = json.loads(payload.decode("utf-8")) if payload else {}
    return frame_type, data

class Client:
    def __init__(self, name):
        self.name = name
        self.sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self.sock.connect((HOST, PORT))
        self.sock.settimeout(5)
        self.seq = 0

    def next_seq(self):
        self.seq += 1
        return self.seq

    def login(self, username, password):
        seq = self.next_seq()
        send_frame(self.sock, "auth.login", {
            "username": username,
            "password": password
        }, seq=seq, frame_type=0x01)
        ft, resp = recv_frame(self.sock)
        assert ft == 0x02, f"Expected RESPONSE(0x02), got {ft}"
        assert resp["type"] == "auth.login_res", f"Expected auth.login_res, got {resp['type']}"
        body = resp["body"]
        assert body["code"] == 0, f"Login failed: {body}"
        print(f"  [{self.name}] 登录成功: user_id={body['user_id']}, nickname={body['nickname']}")
        return body

    def send_chat(self, to_uid, content):
        seq = self.next_seq()
        send_frame(self.sock, "chat.send", {
            "to_user_id": to_uid,
            "content": content,
            "msg_type": 1
        }, seq=seq, frame_type=0x01)
        ft, resp = recv_frame(self.sock)
        assert ft == 0x02, f"Expected RESPONSE, got {ft}"
        body = resp["body"]
        assert body["code"] == 0, f"Send failed: {body}"
        print(f"  [{self.name}] 消息已发送: msg_id={body['msg_id']}, timestamp={body['timestamp']}")
        return body

    def recv_notification(self):
        """接收一条 NOTIFICATION（如 chat.recv）"""
        ft, data = recv_frame(self.sock)
        assert ft == 0x03, f"Expected NOTIFICATION(0x03), got {ft}"
        print(f"  [{self.name}] 收到推送: type={data['type']}")
        return data

    def get_history(self, with_uid, before_msg_id=0, limit=20):
        seq = self.next_seq()
        send_frame(self.sock, "chat.history", {
            "with_user_id": with_uid,
            "before_msg_id": before_msg_id,
            "limit": limit
        }, seq=seq, frame_type=0x01)
        ft, resp = recv_frame(self.sock)
        assert ft == 0x02, f"Expected RESPONSE, got {ft}"
        body = resp["body"]
        assert body["code"] == 0, f"History failed: {body}"
        msgs = body["messages"]
        print(f"  [{self.name}] 拉取历史: {len(msgs)} 条消息")
        for m in msgs:
            print(f"    msg_id={m['msg_id']} from={m['from_user_id']} to={m['to_user_id']}: {m['content']}")
        return msgs

    def close(self):
        self.sock.close()


def main():
    print("=== TinyWeChat Phase 1 验收测试 ===\n")

    # 1. Alice 登录
    print("1) Alice 登录...")
    alice = Client("Alice")
    alice.login("alice", "123456")

    # 2. Bob 登录
    print("\n2) Bob 登录...")
    bob = Client("Bob")
    bob.login("bob", "123456")

    # 3. Alice 发消息给 Bob
    print("\n3) Alice 发送 '你好 Bob!' → Bob...")
    alice.send_chat(to_uid=2, content="你好 Bob!")

    # 4. Bob 应收到推送
    print("\n4) Bob 收到推送...")
    bob.recv_notification()

    # 5. Bob 回复 Alice
    print("\n5) Bob 回复 '你好 Alice!' → Alice...")
    bob.send_chat(to_uid=1, content="你好 Alice!")

    # 6. Alice 收到推送
    print("\n6) Alice 收到推送...")
    alice.recv_notification()

    # 7. Alice 发第二条
    print("\n7) Alice 发送 '今天天气不错' → Bob...")
    alice.send_chat(to_uid=2, content="今天天气不错")

    # 8. Bob 收到第二条
    print("\n8) Bob 收到推送...")
    bob.recv_notification()

    # 9. Bob 拉取历史（alice<->bob 的全部消息）
    print("\n9) Bob 拉取与 Alice 的历史...")
    bob.get_history(with_uid=1)

    # 10. Alice 也拉取历史
    print("\n10) Alice 拉取与 Bob 的历史...")
    alice.get_history(with_uid=2)

    alice.close()
    bob.close()

    print("\n=== 全部测试通过! ===")


if __name__ == "__main__":
    try:
        main()
    except Exception as e:
        print(f"\nFAIL: {e}", file=sys.stderr)
        sys.exit(1)
