#resources:

#https://ankiweb.net/shared/info/2055492159
#https://git.sr.ht/~foosoft/anki-connect


import json
import urllib.request
import sys

def request(action, **params):
    return {'action': action, 'params': params, 'version': 6}

def invoke(action, **params):
    requestJson = json.dumps(request(action, **params)).encode('utf-8')
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

if __name__ == "__main__":
    #sync the anki account
    invoke('sync')

    #finds the due/new cards for the day
    due_cards = invoke('findCards', query='deck:test2 is:due OR is:new')
    if len(due_cards) == 0:
        print("\nCongratulations! You have finished this deck for now.\n")
        sys.exit(0)
    print(f'\nDue cards in test2: {due_cards}')

    #gets the front and back of the due cards
    show_cards = invoke('cardsInfo', cards=due_cards)

    #starts the reviewing 
    for card in show_cards:
        print(f'\nFront: {card['fields']['Front']['value']}')

        input("enter to reveal")
        print(f'Back: {card['fields']['Back']['value']}')
        #Ease is between 1 (Again) and 4 (Easy)
        while True:
            response = int(input("you get it?: "))
            if response == 1 or response == 4: break
            print("Ease is between 1 (Again) and 4 (Easy)")
            
        invoke('answerCards', answers=[{"cardId": card['cardId'], "ease": response}])

