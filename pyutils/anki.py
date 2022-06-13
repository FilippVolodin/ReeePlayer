import genanki
import argparse
import hashlib

def gen_uid(string):
    hash = hashlib.sha1(str.encode(string)).hexdigest()
    return int(hash[:8], 16) // 2

def export(input, media_path, add_voice, add_image, output, deck_name):

    model_fields=[
        {'name': 'Text1'},
        {'name': 'Text2'}]
    
    if add_image:
        model_fields.append({'name': 'Image'});

    if add_voice:
        model_fields.append({'name': 'Voice'});

    if add_voice:
        if add_image:
            qfmt = '{{Image}}<br>{{Voice}}'
        else:
            qfmt = '{{Voice}}'
    else:
        if add_image:
            qfmt = '{{Image}}<br>{{Text1}}'
        else:
            qfmt = '{{Text1}}'        

    afmt = '{{Text1}}<hr>{{Text2}}'

    my_model = genanki.Model(
        74178498,
        'ReeeModel',
        fields=model_fields,
        templates=[
            {
                'name': 'ReeeMediaCard',
                'qfmt': qfmt,
                'afmt': afmt,
            },
        ])

    deck = genanki.Deck(
        gen_uid(deck_name),
        deck_name)

    package = genanki.Package(deck)
    with open(input, encoding='UTF-8') as f:
        while True:
            uid = f.readline()
            text1 = f.readline()
            text2 = f.readline()
            f.readline()

            if not (uid and text1 and text2):
                break

            uid = uid.rstrip('\n')
            text1 = text1.rstrip('\n')
            text2 = text2.rstrip('\n')

            if add_image:
                package.media_files.append(f"{media_path}/{uid}.jpg")
            if add_voice:
                package.media_files.append(f"{media_path}/{uid}.mp3")

            note_fields = [text1, text2]
            if add_image:
                note_fields.append(f'<img src="{uid}.jpg">')
            if add_voice:
                note_fields.append(f'[sound:{uid}.mp3]')

            note = genanki.Note(model=my_model, fields=note_fields)    
            deck.add_note(note)

    package.write_to_file(output)

parser = argparse.ArgumentParser()
parser.add_argument('input', type=str)
parser.add_argument('media_path', type=str)
parser.add_argument('-vc', action="store_true")
parser.add_argument('-tb', action="store_true")
parser.add_argument('output', type=str)
parser.add_argument('deck_name', type=str)
args = parser.parse_args()
export(args.input, args.media_path, args.vc, args.tb, args.output, args.deck_name)
#export("C:/Users/filip/AppData/Local/Temp/clips.txt", "d:/test", True, True, "test4.apkg", "Test4")
