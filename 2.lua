for _, filedir in ipairs(os.filedirs("resources/**")) do
    if os.isfile(filedir) then
        print(format('"%s" "%s"', filedir, filedir))
    end
end
