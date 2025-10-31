#anki api resources: https://git.sr.ht/~foosoft/anki-connect

from flask import Flask, request, jsonify
import json
import urllib.request
import sys

app = Flask(__name__)


def convert_anki_result_to_cards(anki_data):
    raw_cards = anki_data.get("result", [])
    converted = []

    for c in raw_cards:
        card = {
            "id": c.get("cardId"),
            "front": c.get("fields", {}).get("Front", {}).get("value", ""),
            "back": c.get("fields", {}).get("Back", {}).get("value", "")
        }
        converted.append(card)
    return converted

def make_request(action, **params):
    return {'action': action, 'params': params, 'version': 6}

def invoke(action, **params):
    requestJson = json.dumps(make_request(action, **params)).encode('utf-8')
    response = json.load(urllib.request.urlopen(urllib.request.Request('http://127.0.0.1:8765', requestJson)))
    if len(response) != 2:
        raise Exception('response has an unexpected number of fields')
    if 'error' not in response:
        raise Exception('response is missing required error field')
    if 'result' not in response:
        raise Exception('response is missing required result field')
    if response['error'] is not None:
        raise Exception(response['error'])
    return response['result']

@app.route("/boot", methods=["POST"])
def boot():
    #anki part
    invoke('sync')
    due_cards = invoke('findCards', query='deck:test2 is:due OR is:new')
    if len(due_cards) == 0:
        print("\nCongratulations! You have finished this deck for now.\n")
        sys.exit(0)
    print(f'\nDue cards in test2: {due_cards}')
    show_cards = invoke('cardsInfo', cards=due_cards)
    show_cards_send = convert_anki_result_to_cards({"result": show_cards})

    data = request.json
    print("Boot triggered by:", data)
    return jsonify({
        "status": "ok",
        "message": "Study session started",
        "cards": show_cards_send
    }), 200


@app.route("/ease", methods=["POST"])
def ease():
    data = request.json
    print("Ease data received:", data)

    for card in data['results']:
        invoke('answerCards', answers=[{"cardId": card['card_id'], "ease": card['ease']}])

    return jsonify({"status": "ok", "message": "Ease recorded"}), 200

if __name__ == "__main__":
    app.run(host="0.0.0.0", port=8000)
