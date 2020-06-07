from flask import Flask, render_template, request, jsonify
from flask_restful import Resource, Api

app = Flask(__name__)

global led
global tempo
global identifier
led = 0
tempo = 0
identifier = 0

@app.route('/')
def control():
   return render_template('index.html')

@app.route('/status', methods = ['POST', 'GET'])
def status():
   global led
   global tempo
   global identifier
   if request.method == 'POST':
      status = request.form
      led = status['LED']
      tempo = status['tempo']
      identifier = status['ID']
      return render_template("status.html", status = status)
   else:
      return jsonify({'led' : led, 'tempo':tempo, 'id': identifier}), 200

if __name__ == '__main__':
    app.run(host='0.0.0.0',debug=True)
