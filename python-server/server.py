from flask import Flask, render_template, request, jsonify
from flask_restful import Resource, Api
from werkzeug.serving import WSGIRequestHandler
app = Flask(__name__)

global led
global tempo
global identifier
global pot
led = 0
tempo = 0
identifier = 0
pot = 0

@app.route('/')
def control():
   return render_template('index.html')

@app.route('/status', methods = ['POST', 'GET'])
def status():
   global led
   global tempo
   global identifier
   global pot
   if request.method == 'POST':
      status = request.form
      led = status['LED']
      pot = status['POT']
      tempo = status['tempo']
      identifier = status['ID']
      return render_template("status.html", status = status)
   else:
      return jsonify({'digital' : led, 'tempo':tempo, 'id': identifier, 'analogico': pot}), 200

if __name__ == '__main__':
    WSGIRequestHandler.protocol_version = "HTTP/1.1"
    app.run(host='0.0.0.0',debug=True)
