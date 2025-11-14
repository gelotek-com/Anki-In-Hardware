from flask import Flask, request, jsonify, send_from_directory
import json
import urllib.request
import sys

#audio files
import os
from gtts import gTTS
from pydub import AudioSegment
AUDIO_FOLDER = "audioFiles"
os.makedirs(AUDIO_FOLDER, exist_ok=True)


app = Flask(__name__)


def generate_tts_audio(text, filename_base, lang="en"):
    mp3_path = os.path.join(AUDIO_FOLDER, f"{filename_base}.mp3")
    wav_path = os.path.join(AUDIO_FOLDER, f"{filename_base}.wav")

    if os.path.exists(wav_path):
        return wav_path

    tts = gTTS(text=text, lang=lang)
    tts.save(mp3_path)

    audio = AudioSegment.from_mp3(mp3_path)
    audio = audio.set_frame_rate(16000)
    audio = audio.set_channels(1)
    audio = audio.set_sample_width(1)
    audio.export(wav_path, format="wav")

    return wav_path

def convert_anki_result_to_cards(anki_data):
    raw_cards = anki_data.get("result", [])
    converted = []

    for c in raw_cards:
        card_id = c.get("cardId")
        front_text = c.get("fields", {}).get("Front", {}).get("value", "")
        back_text  = c.get("fields", {}).get("Back", {}).get("value", "")

        generate_tts_audio(front_text, f"{card_id}_front")
        generate_tts_audio(back_text, f"{card_id}_back")

        card = {
            "id": card_id,
            "front": front_text,
            "back": back_text
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


@app.route("/audio/<filename>")
def serve_audio(filename):
    return send_from_directory("audioFiles", filename)


@app.route("/ease", methods=["POST"])
def ease():
    data = request.json
    print("Ease data received:", data)

    for card in data['results']:
        invoke('answerCards', answers=[{"cardId": card['card_id'], "ease": card['ease']}])

    return jsonify({"status": "ok", "message": "Ease recorded"}), 200


if __name__ == "__main__":
    app.run(host="0.0.0.0", port=8000)