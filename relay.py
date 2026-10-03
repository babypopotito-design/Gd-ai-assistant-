#!/usr/bin/env python3
import json
import os
import time
import urllib.request
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer

OPENAI_BASE = os.environ.get("OPENAI_API_BASE", "https://api.openai.com/v1").rstrip("/")
OPENAI_KEY = os.environ["OPENAI_API_KEY"]
MODEL = "gpt-5-mini"
last_request = {}

SYSTEM = """You are GD AI Assistant inside the Geometry Dash level editor. Convert the user's request into a concise explanation and safe, concrete editor actions. Coordinates are Geometry Dash editor coordinates; use a small number of actions (maximum 30). Supported objects are spike, block, and decoration. Never claim that actions were applied: they are only a preview until the user taps Apply. If the request is unclear, return an empty actions list and ask a question."""

ACTION_SCHEMA = {
    "type": "json_schema",
    "json_schema": {
        "name": "gd_editor_plan",
        "strict": True,
        "schema": {
            "type": "object",
            "properties": {
                "message": {"type": "string"},
                "actions": {
                    "type": "array",
                    "maxItems": 30,
                    "items": {
                        "type": "object",
                        "properties": {
                            "object": {"type": "string", "enum": ["spike", "block", "decoration"]},
                            "x": {"type": "number", "minimum": 0, "maximum": 100000},
                            "y": {"type": "number", "minimum": 0, "maximum": 2000},
                            "scale": {"type": "number", "minimum": 0.25, "maximum": 4},
                            "rotation": {"type": "number", "minimum": -360, "maximum": 360}
                        },
                        "required": ["object", "x", "y", "scale", "rotation"],
                        "additionalProperties": False
                    }
                }
            },
            "required": ["message", "actions"],
            "additionalProperties": False
        }
    }
}

class Handler(BaseHTTPRequestHandler):
    def log_message(self, *_):
        return

    def send_json(self, status, payload):
        data = json.dumps(payload).encode()
        self.send_response(status)
        self.send_header("Content-Type", "application/json")
        self.send_header("Content-Length", str(len(data)))
        self.send_header("Cache-Control", "no-store")
        self.end_headers()
        self.wfile.write(data)

    def do_GET(self):
        if self.path == "/health":
            self.send_json(200, {"ok": True, "model": MODEL})
        else:
            self.send_json(404, {"error": "not found"})

    def do_POST(self):
        if self.path != "/assist":
            self.send_json(404, {"error": "not found"})
            return
        client = self.client_address[0]
        now = time.time()
        if now - last_request.get(client, 0) < 2:
            self.send_json(429, {"error": "please wait a moment"})
            return
        last_request[client] = now
        try:
            length = min(int(self.headers.get("Content-Length", "0")), 8192)
            body = json.loads(self.rfile.read(length))
            prompt = str(body.get("prompt", "")).strip()
            if not prompt or len(prompt) > 2000:
                self.send_json(400, {"error": "prompt must be 1-2000 characters"})
                return
            payload = json.dumps({
                "model": MODEL,
                "messages": [
                    {"role": "system", "content": SYSTEM},
                    {"role": "user", "content": prompt},
                ],
                "max_completion_tokens": 1000,
                "reasoning": {"effort": "minimal"},
                "response_format": ACTION_SCHEMA,
            }).encode()
            req = urllib.request.Request(
                f"{OPENAI_BASE}/chat/completions",
                data=payload,
                headers={
                    "Authorization": f"Bearer {OPENAI_KEY}",
                    "Content-Type": "application/json",
                },
                method="POST",
            )
            with urllib.request.urlopen(req, timeout=45) as response:
                result = json.loads(response.read())
            text = result["choices"][0]["message"].get("content", "")
            plan = json.loads(text)
            self.send_json(200, plan)
        except Exception:
            self.send_json(502, {"error": "AI relay request failed"})

if __name__ == "__main__":
    ThreadingHTTPServer(("0.0.0.0", int(os.environ.get("PORT", "8080"))), Handler).serve_forever()
