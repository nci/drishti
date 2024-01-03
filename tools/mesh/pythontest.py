import requests

print(requests.post("https://licenseprototype.herokuapp.com/verify", data = 'hello'))
