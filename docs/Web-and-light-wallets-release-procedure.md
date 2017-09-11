## Release Notes
- Checkout and test [graphene-ui master](https://github.com/cryptonomex/graphene-ui)
- Add release notes to `graphene-ui/release-notes.txt`
- Commit your changes

## Open Ledger wallet release:
- Clone [openledger graphene-ui](https://github.com/openledger/graphene-ui)
- Add upstream repo 
  `git remote add cnx https://github.com/cryptonomex/graphene-ui`
- Fetch upstream
  `git fetch cnx`
- Merge upstream/master
  `git merge cnx/master`
- Run `npm install` in `dl/` and in `web/`
- Commit changes to openledger repository
- Tag using in this format: `2.X.YYMMDD`
- Push including tags `git push --tags`
- Build (`cd web/ && npm run build`)
- Open `dist/index.html` and make sure everything is working and the version in status bar matches tag
- Clone `https://github.com/cryptonomex/faucet` and checkout `ol` branch and install gems with bundle command
- In faucet dir run `mina wallet` - this will deploy to 'ol' server (specified in `.ssh/config`)
- Alternatively, copy the dist folder directly to the server: scp dist/* bitshares.openledger.info:/www/current/public/wallet/    
- Open https://bitshares.openledger.info and make sure there are no errors and version matches release tag

## Light wallets
- (one time) Install [NSIS](http://nsis.sourceforge.net/Main_Page)
- `git clone https://github.com/bitshares/bitshares-2-ui` branch `bitshares`
- Add upstream repo `git remote add cnx https://github.com/cryptonomex/graphene-ui`
- Fetch upstream
  `git fetch cnx`
- Merge upstream/master into bitshares branch
  `git merge cnx/master`
- Run `npm install` in `dl/` , `web/` **and** in `electron/`
- Tag it with release version
- Edit `electron/build/package.json` and update version
- Commit your changes and push both commits and tags
- Build it in `web/` via `npm run electron` command
- Goto to `electron/`
- Build light wallet via `npm run release` command
- It will create dmg/deb/exe file in `releases/`
- Make sure created file name matches tag/version
- Run installer and test the app
- Go to bitshares-2 repo
- Update gui_version, commit, tag and push - this will create a new tag
- Open https://github.com/bitshares/bitshares-2/releases, create new release under tag created in previous step
- Specify release notes, upload dmg/deb/exe wallets created earlie

## Bitshares.org wallet and downloads page
- Go to bitshares.gihub.io repo (`git clone https://github.com/bitshares/bitshares.github.io`)
- Copy `bitshares-2-ui/web/dist/*` to `wallet/`
- Edit `_includes/download.html` and update download links and gui release date
- Commit and push
