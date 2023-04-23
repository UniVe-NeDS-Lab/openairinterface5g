from flask import Flask, request
import json
from pprint import pprint
app = Flask(__name__)

@app.route('/', methods=['POST'])
def handle_request():
  # Parse the JSON data in the request
  request_data = request.get_json()

  # Print the parsed data
  pprint(request_data)

  return 'OOK'

if __name__ == '__main__':
  app.run()

