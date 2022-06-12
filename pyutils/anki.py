import genanki
import argparse

def export(input, output, deck_name):

    my_model = genanki.Model(
        74178498,
        'ReeeModel',
        fields=[
            {'name': 'Text 1'},
            {'name': 'Text 2'},
        ],
        templates=[
            {
                'name': 'ReeeCard',
                'qfmt': '{{Text 1}}',
                'afmt': '{{Text 2}}',
            },
        ])

    deck = genanki.Deck(
        50548154,
        deck_name)

    with open(input, encoding='UTF-8') as f:
        while True:
            text1 = f.readline()
            text2 = f.readline()
            sep = f.readline()
            if not (text1 and text2 and sep):
                break

            note = genanki.Note(model=my_model, fields=[text1, text2])    
            deck.add_note(note)

    genanki.Package(deck).write_to_file(output)

parser = argparse.ArgumentParser()
parser.add_argument('input', type=str)
parser.add_argument('output', type=str)
parser.add_argument('deck_name', type=str)
args = parser.parse_args()
export(args.input, args.output, args.deck_name)
