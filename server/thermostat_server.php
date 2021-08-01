<!--/////////////////MAIN///////////////////-->
<!DOCTYPE html>
<html>
<head>
<style>
h1 {text-align: center;}
h2 {text-align: center;}
</style>
</head>

<body>
<h1>ECE 531 Thermostat Control</h1>
<h2>Nathaniel Miller</h2>
<meta http-equiv="refresh" content="200" > 


<!-- Change Schedule -->
<h3>Manual Temperature Set (Reset at Next Schedule Change)</h3>
<form method="post" action="thermostat_status_server.php">
  <table border="0">
    <tr>
      <td>
        <input type="number" name="TEMPERATURE" min="-100" max="150" />
      </td>
      <td>
        <input type="submit" value="set" name="new_temp">
      </td>
    </tr>
  </table>
</form>

<!-- Change Schedule -->
<h3>Add New Thermostat Schedule</h3>
<form method="post" action="thermostat_schedule_server.php">
  <table border="0">
    <tr>
      <td>Day</td>
      <td>Time</td>
      <td>Temperature</td>
    </tr>
    <tr>
      <td>
        <!-- <input type="number" name="DAY" min="1" max="7" /> -->
        <select name="DAY">
          <option value="1">Monday</option>
          <option value="2">Tuesday</option>
          <option value="3">Wednesday</option>
          <option value="4">Thursday</option>
          <option value="5">Friday</option>
          <option value="6">Saturday</option>
	  <option value="7">Sunday</option>
      </select><br>

      </td>
      <td>
        <input type="time" name="TIME" />
      </td>
      <td>
        <input type="number" name="TEMPERATURE" min="-100" max="150" />
      </td>
      <td>
        <input type="submit" value="add" name="submit">
      </td>
    </tr>
  </table>
</form>

<div id="Status">
<script type="text/javascript" src="https://ajax.googleapis.com/ajax/libs/jquery/1.3.0/jquery.min.js"></script>
<script type="text/javascript">
    var auto_refresh = setInterval(
    function ()
    {
    $('#Status').load('schedule.php');
    }, 1000); // refresh every 1000 milliseconds
</script>
</div>


</body>
</html>


