from flask import Flask, render_template, request, jsonify
from flask_restful import Resource, Api

app = Flask(__name__)

global led
global tempo
led = 0
tempo = 0

@app.route('/')
def control():
   return render_template('index.html')

@app.route('/status', methods = ['POST', 'GET'])
def status():
   global led
   global tempo
   if request.method == 'POST':
      status = request.form
      print(status['LED'])
      led = status['LED']
      tempo = status['tempo']
      return render_template("status.html", status = status)
   else:
      return jsonify({'led' : led, 'tempo':tempo}), 200

if __name__ == '__main__':
    app.run(host='0.0.0.0',debug=True)
