defmodule Multicast do
  def help do
    IO.puts """
    --address <address>        Multicast group IP
    --port <port>              Multicast group port
    --interface <interface>    Bind to a specific interface
    --timeout <timeout>        Exit program after specified number of seconds
    --help                     Show this help message
    """
    System.halt(1)
  end

  def is_multicast_address?(address) do
    try do
      {:ok, ip_address} = address |> to_charlist |> :inet.parse_address
      first_octet = ip_address |> elem(0)
      first_octet >= 224 and first_octet <= 239
    rescue
      MatchError -> false
    end
  end

  def get_interface_params(interface_name) do
    {:ok, interfaces} = :inet.getifaddrs()

    interface = Enum.find(interfaces, fn({name, _}) ->
      name == interface_name
    end)

    if interface do
      interface |> elem(1)
    else
      IO.puts "error: Interface not found"
      System.halt(1)
    end
  end

  def get_interface_ipv4(params) do
    address = Enum.find(params, fn({name, value}) ->
      name |> to_string |> String.equivalent?('addr') and
      value |> tuple_size == 4
    end)

    if address do
      address |> elem(1)
    else
      IO.puts "error: Interface has no IP address"
      System.halt(1)
    end
  end

  def get_interface_address(interface_name) do
    interface_name
    |> get_interface_params
    |> get_interface_ipv4
  end

  def process([]) do
    help()
  end

  def process(options) do
    if ! options[:address] do
      IO.puts "error: Address is a required field"
      System.halt(1)
    end

    if ! is_multicast_address?(options[:address]) do
      IO.puts "error: Address is not a valid multicast address"
      System.halt(1)
    end

    if ! options[:port] do
      IO.puts "error: Port is a required field"
      System.halt(1)
    end

    if options[:port] <= 1024 or options[:port] > 65535 do
      IO.puts "error: Port is out of range"
      System.halt(1)
    end

    if ! options[:port] do
      IO.puts "error: Interface is a required field"
      System.halt(1)
    end

    {:ok, group_address} = options[:address] |> to_charlist |> :inet.parse_address
    local_address = options[:interface] |> to_charlist |> get_interface_address
    local_address_s = local_address |> Tuple.to_list |> Enum.join(".")

    [
      group_address: group_address,
      group_address_s: options[:address],
      group_port: options[:port],
      interface: options[:interface],
      local_address: local_address,
      local_address_s: local_address_s,
      timeout: options[:timeout],
      socket: nil,
      message_count: 0,
      start_time: Time.utc_now
    ]
  end

  def subscribe(options) do
    udp_options = [
      :binary,
      active: false,
      add_membership: {options[:group_address], options[:local_address]},
      multicast_if: options[:local_address],
      multicast_loop: false,
      reuseaddr: true
    ]

    IO.puts "subscriber=#{options[:local_address_s]} destination=#{options[:group_address_s]}:#{options[:group_port]}"
    {:ok, socket} = :gen_udp.open(options[:group_port], udp_options)
    [socket: socket] ++ options
  end

  def get_runtime(start_time) do
    Time.diff(Time.utc_now, start_time)
  end

  def has_timed_out?(start_time, timeout) do
    if timeout, do: get_runtime(start_time) >= timeout, else: false
  end

  def show_exit_statistics(start_time, message_counter) do
    run_time = start_time |> get_runtime
    IO.puts "\nExiting after #{run_time} seconds and #{message_counter} messages."
    System.halt(0)
  end

  def recieve(options, message_counter) do
    case :gen_udp.recv(options[:socket], 0, 1) do
      { :ok, data } ->
        time_now = DateTime.utc_now() |> DateTime.to_string
        source_address = data |> elem(0) |> Tuple.to_list |> Enum.join(".")
        source_port = data |> elem(1)
        data_size = data |> elem(2) |> String.length

        IO.puts  "#{time_now} source=#{source_address}:#{source_port} destination=#{options[:group_address_s]}:#{options[:group_port]} size=#{data_size}"
        message_counter = message_counter + 1

        if ! has_timed_out?(options[:start_time], options[:timeout]) do
          recieve(options, message_counter)
        end

      {:error, :timeout} ->
        if ! has_timed_out?(options[:start_time], options[:timeout]) do
          recieve(options, message_counter)
        end
    end

    show_exit_statistics(options[:start_time], message_counter)
  end

  defp parse_args(args) do
    {options, _, invalid} = OptionParser.parse(args,
      strict: [
        address: :string,
        port: :integer,
        interface: :string,
        timeout: :integer,
        help: :boolean
      ]
    )

    if options[:help] do
      help()
    end

    if length(invalid) > 0 do
      IO.puts "error: Unknown argument\n"
      help()
    end

    options
  end

  def main(args) do
    args
    |> parse_args
    |> process
    |> subscribe
    |> recieve(0)
  end
end
