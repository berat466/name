from flask import flask requesr,jsonify
from flask_cors import cors
import psycopg2,os

app=flask(_name_)
CORS(app)
DATABASE_url","postgresql://hello_cloud3_user:eYLe8yk3gYq6VpVRTMXkhvoWcsLGjRoa@dpg-d71s34paae7s73flngag-a/hello_cloud3"
)

def connect_db():
return psycopg2.connect(DATABASE_URL)
@app.route("/ziyaretciler", methods=["GET", "POST"])
def ziyaretciler():
conn = connect_db()
cur = conn.cursor()
cur.execute("CREATE TABLE IF NOT EXISTS ziyaretciler (id SERIAL PRIMARY KEY, isim
TEXT)")
if request.method == "POST":
isim = request.json.get("isim")
if isim:
cur.execute("INSERT INTO ziyaretciler (isim) VALUES (%s)", (isim,))
conn.commit()
cur.execute("SELECT isim FROM ziyaretciler ORDER BY id DESC LIMIT 10")
isimler = [row[0] for row in cur.fetchall()]
cur.close()
conn.close()
return jsonify(isimler)
if __name__ == "__main__":
app.run(host="0.0.0.0", port=5001)
