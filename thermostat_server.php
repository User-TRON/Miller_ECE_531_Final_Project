<?php

  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  function setup_sql_connection(){
    //echo "Setup Database and Connect\n";

    include "../inc/dbinfo.inc"; 
    $sql_connection = new mysqli(DB_SERVER, DB_USERNAME, DB_PASSWORD);

    if (mysqli_connect_errno()) echo "Failed to connect to MySQL: " . mysqli_connect_error();
    //echo "Connection Successful to MYSQL Database";

    $server_database = mysqli_select_db($sql_connection, DB_DATABASE);

    $query = "CREATE TABLE IF NOT EXISTS temp_schedule (
        ID INT NOT NULL PRIMARY KEY AUTO_INCREMENT, 
        DAY ENUM('SUNDAY', 'MONDAY', 'TUESDAY', 'WEDNESDAY', 'THURSDAY', 'FRIDAY', 'SATURDAY') NOT NULL,
        SET_TIME TIME NOT NULL,
        TEMP_SET INT NOT NULL)";

    if(!mysqli_query($sql_connection, $query)) echo("<p>Error creating table.</p>");

    //echo "Table temp_schedule is good";
    $query = "CREATE TABLE IF NOT EXISTS status (
        ID INT NOT NULL PRIMARY KEY AUTO_INCREMENT, 
        TIME_LAST_PROGRAMMED BIGINT UNSIGNED NOT NULL,
        TIME_LAST_UPDATE BIGINT UNSIGNED NOT NULL,
        CURR_TEMP INT NOT NULL,
        SET_TEMP INT NOT NULL,
        POWER INT NOT NULL,
        NEW_TEMP INT)";

    if(!mysqli_query($sql_connection, $query)) echo("<p>Error creating table.</p>");

    //echo "Table status is good";

    return $sql_connection;
  }

  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  function get_thermostat_schedule($sql_connection){
    return mysqli_query($sql_connection, "SELECT * FROM temp_schedule");
  }

  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  function get_status($sql_connection){
    return mysqli_query($sql_connection, "SELECT * FROM status");
  }

  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  function print_thermostat_program_table($result){
//    echo "print table";

    while($query_data = mysqli_fetch_row($result)) {
      echo "<tr>";
      echo "<td>",$query_data[1], "</td>",
           "<td>",$query_data[2], "</td>",
           "<td>",$query_data[3], "</td?";
      echo "</tr>";
    }
  }

  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  function print_current_time(){
    $query_data = mysqli_fetch_row($result);
    echo date('F j, Y, g:i a T');
  }

  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  function print_last_updated_timestamp($result){
    $query_data = mysqli_fetch_row($result);
    echo date('F j, Y, g:i a T', $query_data[2]);
  }

  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  function print_current_temp($result){
    $query_data = mysqli_fetch_row($result);
    echo $query_data[3];
  }

  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  function print_set_temp($result){
    $query_data = mysqli_fetch_row($result);
    echo $query_data[4];
  }

  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  function print_power($result){
    $query_data = mysqli_fetch_row($result);
    echo $query_data[5];
  }

?>

<!--/////////////////MAIN///////////////////-->
<!DOCTYPE html>
<html>
<body>
<h1>ECE 531 Thermostat Control</h1>

<?php
  $sql_connection = setup_sql_connection();
  date_default_timezone_set('America/Denver');
?>


<h2>Current Temperature at Thermostat</h2>
<?php
  $result = get_status($sql_connection);
  print_current_temp($result);
?>

<h2>Current Set Temperature</h2>
<?php
  $result = get_status($sql_connection);
  print_set_temp($result);
?>

<!-- Change Schedule -->
<form method="post" action="thermostat_status_server.php">
  <table border="0">
    <tr>
      <td>Change Temperature Until Next Schedule Change</td>
    </tr>
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


<h2>Current Power</h2>
<?php
  $result = get_status($sql_connection);
  print_power($result);
?>

<h2>Current Time at Thermostat</h2>
<?php
  print_current_time();
?>

<h2>Time of Last Update from Thermostat</h2>
<?php
  $result = get_status($sql_connection);
  print_last_updated_timestamp($result);
?>

<h2>Thermostat Schedule</h2>
<table border="1" cellpadding="2" cellspacing="2">
  <tr>
    <td>DAY</td>
    <td>TIME</td>
    <td>TEMPERATURE</td>
  </tr>
<?php
  $result = get_thermostat_schedule($sql_connection);
  print_thermostat_program_table($result);
?>

</table>

<!-- Change Schedule -->
<h2>Add New Thermostat Schedule</h2>
<form method="post" action="thermostat_schedule_server.php">
  <table border="0">
    <tr>
      <td>Day</td>
      <td>Time</td>
      <td>Temperature</td>
    </tr>
    <tr>
      <td>
        <input type="number" name="DAY" min="1" max="7" />
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


<!-- Clean up. -->
<?php
  mysqli_free_result($result);
  mysqli_close($sql_connection);
?>

</body>
</html>


