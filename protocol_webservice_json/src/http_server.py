from flask import Flask, request, jsonify

app = Flask(__name__)


@app.route("/api/test", methods=["POST"])
def result():
    content = request.get_json(silent=True)
    print(content)
    return jsonify(message="OK")


if __name__ == "__main__":
    app.run(host="192.168.254.2", port=8080)
