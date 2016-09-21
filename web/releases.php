<?php

echo "<font size='6px' color=#005f7f>";
echo "<center>Visteon Software Releases<br></center>";
echo "</font><br>";

echo '<p align="center"> <a href="main.php">Home Page</a></p>';

session_start();
$username=$_SESSION['username'];
$password=$_SESSION['password'];
$database=$_SESSION['database'];

$link = mysqli_connect(localhost,$username,$password, $database);


$tableq = "SHOW TABLES FROM $database";
$tabler = mysqli_query($link, $tableq);
if(!$tabler) {
	echo "DB Error, could not list tables\n";
	echo 'MySQL Error: ' . mysql_error();
	exit;
}

while($table_row = mysqli_fetch_row($tabler)) {
	if($table_row[0] == "sotatbl")
		continue;
	if(0 == strncmp($table_row[0], "ecus_", 5))
		continue;

	/* print table title */
	echo "<br><font size='4px' color=#ff5f00>";
	echo "$table_row[0]\n";
	echo "</font>";

	$query="select * from sotadb.$table_row[0] ORDER by sw_version DESC";
	$swreleasetbl=mysqli_query($link, $query);

	$rel_rows=mysqli_num_rows($swreleasetbl);
	$rel_cols=mysqli_num_fields($swreleasetbl);

	/* print vehicles - HEADER */
	echo "<table border=1><tr>";
	$i=0;while ($i < $rel_cols) {
		$meta = mysqli_fetch_field($swreleasetbl);
		echo "<th>$meta->name</th>";
		$i++;
	}
	echo "</tr>";

	/* print vehicles - DATA */
	$i=0;while ($i < $rel_rows) {
		$row=mysqli_fetch_row($swreleasetbl);
		echo "<tr>";
		$j=0;while ($j < $rel_cols) {
			echo "<td>$row[$j]</td>";
			$j++;
		}
		echo "</tr>";
		$i++;
	}
	echo "</table>";

	mysqli_free_result($swreleasetbl);
}

mysqli_free_result($tabler);

mysqli_close();

?>
