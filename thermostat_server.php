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
        NEW_TEMP INT NOT NULL)";

    if(!mysqli_query($sql_connection, $query)) echo("<p>Error creating table.</p>");

    //echo "Table status is good";

    return $sql_connection;
  }

  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  function print_thermostat_program_table($sql_connection){
//    echo "print table";
    $result = mysqli_query($sql_connection, "SELECT * FROM temp_schedule");

    while($query_data = mysqli_fetch_row($result)) {
      echo "<tr>";
      echo "<td>",$query_data[1], "</td>",
           "<td>",$query_data[2], "</td>",
           "<td>",$query_data[3], "</td?";
      echo "</tr>";
    }
  }

  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  function print_last_updated_timestamp($sql_connection){
    $result = mysqli_query($sql_connection, "SELECT * FROM status");
    $query_data = mysqli_fetch_row($result);
    echo $query_data[3];
  }

  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  function print_current_temp($sql_connection){
    $result = mysqli_query($sql_connection, "SELECT * FROM status");
    $query_data = mysqli_fetch_row($result);
    echo $query_data[4];
  }
?>

<!--/////////////////MAIN///////////////////-->
<!DOCTYPE html>
<html>
<body>
<h1>ECE 531 Thermostat Control</h1>

<?php
  $sql_connection = setup_sql_connection();
?>


<!-- Input form -->
<!--
<form action="<PHP echo $_SERVER['SCRIPT_NAME'] ?>" method="PUT">
  <table border="0">
    <tr>
      <td>ID</td>
      <td>JSON</td>
    </tr>
    <tr>
      <td>
        <input type="text" name="id" maxlength="45" size="30" />
      </td>
      <td>
        <input type="text" name="json" maxlength="90" size="60" />
      </td>
      <td>
        <input type="submit" value="Add Data" />
      </td>
    </tr>
  </table>
</form>
-->

<h2>Current Temperature at Thermostat</h2>
<?php
  print_current_temp($sql_connection);
?>

<h2>Time of Last Update from Thermostat</h2>
<?php
  print_last_updated_timestamp($sql_connection);
?>

<h2>Thermostat Schedule</h2>
<table border="1" cellpadding="2" cellspacing="2">
  <tr>
    <td>DAY</td>
    <td>TIME</td>
    <td>TEMPERATURE</td>
  </tr>
<?php
  print_thermostat_program_table($sql_connection);
?>

</table>

<!-- Clean up. -->
<?php
//  mysqli_free_result($result);
  mysqli_close($sql_connection);
?>

</body>
</html>


